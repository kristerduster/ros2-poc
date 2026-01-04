#include "std_msgs/msg/string.hpp"
#include "rclcpp/rclcpp.hpp"

using std::placeholders::_1;

class EditingSubscriber : public rclcpp::Node
{
public:
    EditingSubscriber()
        : Node("edited_subscriber_node")
    {
        publisher_ = this->create_publisher<std_msgs::msg::String>("edited_topic", 10);
        subscription_ = this->create_subscription<std_msgs::msg::String>(
            "communication_topic", 10, std::bind(&EditingSubscriber::on_message, this, _1));
    }

private:
    void on_message(const std_msgs::msg::String &msg)
    {
        auto edited = std_msgs::msg::String();
        edited.data = msg.data + " edit";

        RCLCPP_INFO(this->get_logger(), "I heard: '%s'", msg.data.c_str());
        RCLCPP_INFO(this->get_logger(), "Publishing edited message: '%s'", edited.data.c_str());
        publisher_->publish(edited);
    }

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<EditingSubscriber>());
    rclcpp::shutdown();
    return 0;
}
