#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"

#include "cv_bridge/cv_bridge.h"
#include "image_transport/image_transport.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rcl_interfaces/msg/parameter_descriptor.hpp"
#include "sensor_msgs/msg/image.hpp"

class ImagePublisher : public rclcpp::Node
{
public:
  ImagePublisher()
      : rclcpp::Node("image_publisher_node"), running_(true)
  {
    // Declare ROS parameters with defaults; these can be overridden via CLI/launch
    rcl_interfaces::msg::ParameterDescriptor src_desc;
    src_desc.dynamic_typing = true; // allow int or string for source
    declare_parameter("source", rclcpp::ParameterValue("0"), src_desc);
    declare_parameter<int>("frame_width", 640);
    declare_parameter<int>("frame_height", 480);
    declare_parameter<double>("fps", 0.0); // 0 means uncapped
    declare_parameter<std::string>("encoding", "bgr8");
    declare_parameter<std::string>("topic", "camera/image_raw");
    declare_parameter<int>("queue_size", 10);
    declare_parameter<bool>("show_image", true);

    // Reads params into member vars
    rclcpp::Parameter source_param;
    get_parameter("source", source_param);
    source_param_ = parameter_to_string(source_param);
    get_parameter("frame_width", frame_width_);
    get_parameter("frame_height", frame_height_);
    get_parameter("fps", fps_);
    get_parameter("encoding", encoding_);
    get_parameter("topic", topic_);
    get_parameter("queue_size", queue_size_);
    get_parameter("show_image", show_image_);

    // Image transport publisher on topic_ using a sensor-data QoS profile
    auto qos = rclcpp::SensorDataQoS().keep_last(queue_size_);
    // Pass raw node pointer; Humble's create_publisher expects rclcpp::Node*
    image_pub_ = image_transport::create_publisher(this, topic_, qos.get_rmw_qos_profile());

    // Open the camera or file source before spinning
    if (!open_capture())
    {
      RCLCPP_ERROR(get_logger(), "Failed to open source: %s", source_param_.c_str());
      throw std::runtime_error("VideoCapture open failed");
    }

    // Optional OpenCV preview window
    if (show_image_)
    {
      cv::namedWindow(window_name_, cv::WINDOW_AUTOSIZE);
    }

    // Start background thread that grabs frames and publishes
    capture_thread_ = std::thread(&ImagePublisher::capture_loop, this);
  }

  // Destructor stops loop, joins thread, closes window
  ~ImagePublisher() override
  {
    running_.store(false);
    if (capture_thread_.joinable())
    {
      capture_thread_.join();
    }
    if (show_image_)
    {
      cv::destroyWindow(window_name_);
    }
  }

private:
  bool open_capture()
  {
    // Determine if source param is an integer camera index or path (live vs prerecorded)
    bool numeric = !source_param_.empty() &&
                   std::all_of(source_param_.begin(), source_param_.end(), ::isdigit);

    if (numeric)
    {
      int index = std::stoi(source_param_);
      RCLCPP_INFO(get_logger(), "Attempting to open camera device index: %d with V4L2 backend", index);
      cap_.open(index, cv::CAP_V4L2);
    }
    else
    {
      RCLCPP_INFO(get_logger(), "Attempting to open file source: %s", source_param_.c_str());
      cap_.open(source_param_);
    }

    if (!cap_.isOpened())
    {
      RCLCPP_ERROR(get_logger(), "Failed to open video source: %s. Check device exists and is accessible.", source_param_.c_str());
      return false;
    }

    RCLCPP_INFO(get_logger(), "Video source opened successfully");

    // For numeric (camera) sources, request MJPEG format explicitly to work around driver issues
    if (numeric)
    {
      int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
      cap_.set(cv::CAP_PROP_FOURCC, fourcc);
      RCLCPP_INFO(get_logger(), "Requested MJPEG codec for camera");
    }

    if (frame_width_ > 0)
    {
      cap_.set(cv::CAP_PROP_FRAME_WIDTH, frame_width_);
    }
    if (frame_height_ > 0)
    {
      cap_.set(cv::CAP_PROP_FRAME_HEIGHT, frame_height_);
    }
    if (fps_ > 0.0)
    {
      cap_.set(cv::CAP_PROP_FPS, fps_);
    }

    // Log actual properties after setting
    double actual_width = cap_.get(cv::CAP_PROP_FRAME_WIDTH);
    double actual_height = cap_.get(cv::CAP_PROP_FRAME_HEIGHT);
    double actual_fps = cap_.get(cv::CAP_PROP_FPS);
    int backend_id = static_cast<int>(cap_.get(cv::CAP_PROP_BACKEND));

    RCLCPP_INFO(get_logger(), "Opened with backend=%d, actual resolution=%.0fx%.0f, actual fps=%.1f",
                backend_id, actual_width, actual_height, actual_fps);

    if (actual_width <= 0 || actual_height <= 0)
    {
      RCLCPP_WARN(get_logger(), "Warning: Could not determine frame dimensions. Device may not be ready.");
    }

    return true;
  }

  void capture_loop()
  {
    // If fps_ > 0, enforce a publish rate using rclcpp::Rate
    std::optional<rclcpp::Rate> rate;
    if (fps_ > 0.0)
    {
      rate.emplace(fps_);
    }

    // FPS measurement variables
    int frame_count = 0;
    auto fps_start = std::chrono::steady_clock::now();
    const int fps_report_interval = 100; // report every N frames

    while (running_.load() && rclcpp::ok())
    {
      cv::Mat frame;
      if (!cap_.read(frame) || frame.empty())
      {
        // For file sources, break at end-of-stream; for cameras, keep trying.
        if (!is_camera_source())
        {
          RCLCPP_INFO(get_logger(), "End of stream reached for file source");
          break;
        }
        RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000, "Failed to grab frame; retrying");
        continue;
      }

      // Convert OpenCV image to ROS Image message and publish
      auto msg = cv_bridge::CvImage(std_msgs::msg::Header(), encoding_, frame).toImageMsg();
      msg->header.stamp = now();
      image_pub_.publish(*msg);

      // Optional live preview
      if (show_image_)
      {
        cv::imshow(window_name_, frame);
        cv::waitKey(1);
      }

      // FPS measurement
      frame_count++;
      if (frame_count % fps_report_interval == 0)
      {
        auto fps_end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = fps_end - fps_start;
        double measured_fps = fps_report_interval / elapsed.count();
        RCLCPP_INFO(get_logger(), "Publishing at %.1f FPS (uncapped)", measured_fps);
        fps_start = fps_end;
      }

      // Sleep to respect requested FPS when capped
      if (rate.has_value())
      {
        rate->sleep();
      }
    }

    // exiting
    running_.store(false);
  }

  bool is_camera_source() const
  {
    // true if camera index, false if path
    return !source_param_.empty() &&
           std::all_of(source_param_.begin(), source_param_.end(), ::isdigit);
  }

  static std::string parameter_to_string(const rclcpp::Parameter &param)
  {
    if (param.get_type() == rclcpp::ParameterType::PARAMETER_INTEGER)
    {
      return std::to_string(param.as_int());
    }
    return param.as_string();
  }

  // member fields
  std::string source_param_;
  int frame_width_;
  int frame_height_;
  double fps_;
  std::string encoding_;
  std::string topic_;
  int queue_size_;
  bool show_image_;

  std::atomic<bool> running_;
  cv::VideoCapture cap_;
  image_transport::Publisher image_pub_;
  std::thread capture_thread_;
  const std::string window_name_ = "image_publisher_debug";
};

int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);
  try
  {
    auto node = std::make_shared<ImagePublisher>();
    rclcpp::spin(node);
  }
  catch (const std::exception &ex)
  {
    RCLCPP_FATAL(rclcpp::get_logger("image_publisher_node"), "Exception: %s", ex.what());
  }
  rclcpp::shutdown();
  return 0;
}
// in powershell admin, run: usbipd attach --busid <BUSID> -w Ubuntu-22.04
// to detach run: usbipd detach --busid <BUSID 1-6>
// usage: ros2 run sub_pub image_publisher_node --ros-args -p source:="/mnt/c/Users/krist/Hui Lab/DropShop-Python/video_data/test_videos/Rainbow 11-11-22.m4v" -p fps:=0.0 -p show_image:=true
// OR: ros2 run sub_pub image_publisher_node --ros-args -p source:=0 -p fps:=0.0 -p show_image:=true