#include "sensors/bumpers_subscriber.hpp"

using namespace std::placeholders;



BumpersSubscriber::BumpersSubscriber(rclcpp::Node* node) : node_(node) {
    bumpers_subscriber_ = node_->create_subscription<robot_interfaces::msg::BumpersStates>(
            "bumpers", 10, std::bind(&BumpersSubscriber::callbackBumpers, this, _1));

    RCLCPP_INFO(node_->get_logger(), "Bumpers subscriber node has been started.");
}

// getter for bumper states
bool BumpersSubscriber::isLeftPressed() {
    return left_pressed_;
}

bool BumpersSubscriber::isRightPressed() {
    return right_pressed_;
}

bool BumpersSubscriber::wasLeftPressed() {
    return was_left_pressed_;
}

bool BumpersSubscriber::wasRightPressed() {
    return was_right_pressed_;
}

void BumpersSubscriber::callbackBumpers(const robot_interfaces::msg::BumpersStates::SharedPtr msg) {
    left_pressed_ = msg->left_bumper;
    right_pressed_ = msg->right_bumper;
    if (!was_left_pressed_ && left_pressed_) {
        was_left_pressed_ = true;
    }
    if (!was_right_pressed_ && right_pressed_) {
        was_right_pressed_ = true;
    }
    //RCLCPP_INFO(node_->get_logger(), "Right bumper = %d", msg->right_bumper);
    //RCLCPP_INFO(node_->get_logger(), "Left bumper = %d", msg->left_bumper);
}
