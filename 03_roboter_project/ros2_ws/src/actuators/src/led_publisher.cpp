#include "actuators/led_publisher.hpp"



LedPublisher::LedPublisher(rclcpp::Node* node) : node_(node) {
    led_publisher_ = node->create_publisher<std_msgs::msg::Bool>("led_state", 10);
    RCLCPP_INFO(node_->get_logger(), "LED Publisher started");
}

void LedPublisher::setLed(bool on) {

    if (on == last_state_) {
        return;
    }

    last_state_ = on;
    std_msgs::msg::Bool msg;
    msg.data = on;
    led_publisher_->publish(msg);

    RCLCPP_INFO(node_->get_logger(), "LED: %s", on ? "ON" : "OFF");
}
