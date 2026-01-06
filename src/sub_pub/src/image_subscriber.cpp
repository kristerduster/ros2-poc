#include <memory>
#include <string>

#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include "cv_bridge/cv_bridge.h"
#include "image_transport/image_transport.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"

class ImageSubscriber : public rclcpp::Node
{
public:
    ImageSubscriber()
        : rclcpp::Node("image_subscriber_node")
    {
        declare_parameter<std::string>("topic", "camera/image_raw");
        declare_parameter<std::string>("window_name", "Image Subscriber");
        declare_parameter<int>("queue_size", 10);

        get_parameter("topic", topic_);
        get_parameter("window_name", window_name_);
        get_parameter("queue_size", queue_size_);

        auto qos = rclcpp::SensorDataQoS().keep_last(queue_size_);

        image_sub_ = image_transport::create_subscription(
            this,
            topic_,
            std::bind(&ImageSubscriber::image_callback, this, std::placeholders::_1),
            "raw",
            qos.get_rmw_qos_profile());

        cv::namedWindow(window_name_, cv::WINDOW_AUTOSIZE);

        RCLCPP_INFO(get_logger(), "Image subscriber started, listening on topic: %s", topic_.c_str());

        frame_count_ = 0;
        fps_start_ = std::chrono::steady_clock::now();
    }

    ~ImageSubscriber() override
    {
        cv::destroyWindow(window_name_);
    }

private:
    void image_callback(const sensor_msgs::msg::Image::ConstSharedPtr &msg)
    {
        try
        {
            // Convert ROS Image message to OpenCV Mat
            cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);

            // Display the frame
            cv::imshow(window_name_, cv_ptr->image);
            cv::waitKey(1);

            // FPS measurement
            frame_count_++;
            if (frame_count_ % 100 == 0)
            {
                auto fps_end = std::chrono::steady_clock::now();
                std::chrono::duration<double> elapsed = fps_end - fps_start_;
                double measured_fps = 100.0 / elapsed.count();
                RCLCPP_INFO(get_logger(), "Receiving at %.1f FPS", measured_fps);
                fps_start_ = fps_end;
            }
        }
        catch (const cv_bridge::Exception &e)
        {
            RCLCPP_ERROR(get_logger(), "cv_bridge exception: %s", e.what());
        }
    }

    std::string topic_;
    std::string window_name_;
    int queue_size_;

    image_transport::Subscriber image_sub_;

    int frame_count_;
    std::chrono::steady_clock::time_point fps_start_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ImageSubscriber>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
