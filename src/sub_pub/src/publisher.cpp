#include <string>
#include <chrono>
#include <memory>

// provides set of predefined class templates for fn objs
#include <functional>

#include "std_msgs/msg/string.hpp"
// C++ ROS Client Library API
#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;

// define class that inherits from ROS2 class called "Node"
class SimplePublisher : public rclcpp::Node
{
    public:
    // constructor
	SimplePublisher()
	: Node("publisher_node")
	{
	    // counter value is private var
	    CounterValue = 0;
	    
	    // publisherObject is private var
	    // create publisher object, specifying: 
	    // - message type: "std_msgs::msg::String"
	    // - topic name "communication_topic" (must match topic name in subscriber)
	    // - buffer size: 20
	    publisherObject = this->create_publisher<std_msgs::msg::String>("communication_topic", 20);
	    
	    // create timer obj with args:
	    // - first arg is time interval b/n callback triggers
	    // - second arg is user defined callback fn defined as private var
	    timerObject = this->create_wall_timer(1000ms, std::bind(&SimplePublisher::callbackFunction, this));
	}
    
    private: 
    // user defined callback fn that creates and publishes messages
        void callbackFunction()
        {
            // inc counter
            CounterValue++;
            // create ros2 msg
            auto message = std_msgs::msg::String();
            message.data = "Message Number: " + std::to_string(CounterValue);
            
            // log/print msg in publisher terminal
            RCLCPP_INFO(this->get_logger(), "Publishing message: '%s'", message.data.c_str());
            // publish msg 
            publisherObject->publish(message);
        }
        // timer obj
        rclcpp::TimerBase::SharedPtr timerObject;
        // publisher obj
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisherObject;
        // msg counter
        int CounterValue;
};


int main(int argc, char * argv[])
{
  // init ROS2
  rclcpp::init(argc, argv);
  // spin up node, callback fn is called
  rclcpp::spin(std::make_shared<SimplePublisher>());
  // shutdown after ctrl+c
  rclcpp::shutdown();
  return 0;
}
            
            
            
            
            
            
	    
	    
	    
