#include "actuators/oled_publisher.hpp"



OledPublisher::OledPublisher(rclcpp::Node* node) : node_(node) {
    oled_publisher_ = node->create_publisher<std_msgs::msg::String>("oled_text", 10);
    RCLCPP_INFO(node_->get_logger(), "OLED Publisher started");
}

void OledPublisher::setText(const std::string &text) {

    if (text == last_text_) {
        return;
    }

    last_text_ = text;
    std_msgs::msg::String msg;
    msg.data = text;
    oled_publisher_->publish(msg);

    RCLCPP_INFO(node_->get_logger(), "OLED: %s", text.c_str());
}
