#include "std_msgs/msg/string.hpp"
#include "rclcpp/rclcpp.hpp"

using std::placeholders::_1;

class EditedListener : public rclcpp::Node
{
public:
    EditedListener()
        : Node("edited_listener_node")
    {
        subscription_ = this->create_subscription<std_msgs::msg::String>(
            "edited_topic", 10, std::bind(&EditedListener::on_message, this, _1));
    }

private:
    void on_message(const std_msgs::msg::String &msg) const
    {
        RCLCPP_INFO(this->get_logger(), "Edited listener heard: '%s'", msg.data.c_str());
    }

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<EditedListener>());
    rclcpp::shutdown();
    return 0;
}
