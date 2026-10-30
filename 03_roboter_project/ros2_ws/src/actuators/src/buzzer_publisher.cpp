#include "actuators/buzzer_publisher.hpp"



BuzzerPublisher::BuzzerPublisher(rclcpp::Node* node) : node_(node){
    buzzer_publisher_ = node->create_publisher<std_msgs::msg::Bool>("buzzer_state", 10);
    RCLCPP_INFO(node_->get_logger(), "Buzzer Publisher started");
}


void BuzzerPublisher::setBuzzer(bool on) {

    if (on == last_state_) {
        return;
    }

    last_state_ = on;
    std_msgs::msg::Bool msg;
    msg.data = on;
    buzzer_publisher_->publish(msg);
    RCLCPP_INFO(node_->get_logger(), "Buzzer: %s", on ? "ON" : "OFF");
}
