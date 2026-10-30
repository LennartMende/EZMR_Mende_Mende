#include "actuators/motors_publisher.hpp"



MotorsPublisher::MotorsPublisher(rclcpp::Node* node) : node_(node) {
    motors_publisher_ = node->create_publisher<robot_interfaces::msg::MotorsCmd>("motors_cmd", 10);
    RCLCPP_INFO(node_->get_logger(), "Motors Publisher started");
}


void MotorsPublisher::setMotion(bool dir_left, bool dir_right, uint16_t steps) {

    last_dir_left_ = dir_left;
    last_dir_right_ = dir_right;
    last_steps_ = steps;

    robot_interfaces::msg::MotorsCmd msg;

    msg.dir_left = dir_left;
    msg.dir_right = dir_right;
    msg.steps = steps;

    motors_publisher_->publish(msg);

    RCLCPP_INFO(node_->get_logger(), "Motion command: left=%d right=%d steps=%d", dir_left, dir_right, steps);
}
