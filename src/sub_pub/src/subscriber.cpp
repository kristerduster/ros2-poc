#include "std_msgs/msg/string.hpp"
#include "rclcpp/rclcpp.hpp"

// need this for std::bind
using std::placeholders::_1;

// inherits from Node
class SimpleSubscriber : public rclcpp::Node
{
    public:
        SimpleSubscriber()
        : Node("subscriber_node")
        {
          // "communication_topic" is topic name - must match topic name in subscriber node
          // second input is user defined callback fn 
          subscriberObject = this->create_subscription<std_msgs::msg::String>(
          "communication_topic", 10, std::bind(&SimpleSubscriber::callBackFunction, this, _1));
        }
        
    private:
        // callback fn receives messages of type "std_msgs::msg::String"
        void callBackFunction(const std_msgs::msg::String & msg) const
        {
            // print msg in subscriber terminal
            RCLCPP_INFO(this->get_logger(), "I heard: '%s'", msg.data.c_str());
        }
        rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscriberObject;
};

int main(int argc, char *argv[])
{
  // init ROS2
  rclcpp::init(argc, argv);
  // spinup node, callback fn is called
  rclcpp::spin(std::make_shared<SimpleSubscriber>());
  // shutdown after ctrl+c pressed
  rclcpp::shutdown();
  return 0;
}
  
