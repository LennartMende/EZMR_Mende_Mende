#pragma once

#include "rclcpp/rclcpp.hpp"
#include "robot_interfaces/msg/bumpers_states.hpp"

class BumpersSubscriber {

public:
    BumpersSubscriber(rclcpp::Node* node);
    bool isLeftPressed();
    bool isRightPressed();
    bool wasLeftPressed();
    bool wasRightPressed();

private:
    void callbackBumpers(const robot_interfaces::msg::BumpersStates::SharedPtr msg);
    rclcpp::Node* node_;
    bool left_pressed_ = false;
    bool right_pressed_ = false;
    bool was_left_pressed_ = false;
    bool was_right_pressed_ = false;
    rclcpp::Subscription<robot_interfaces::msg::BumpersStates>::SharedPtr bumpers_subscriber_;
};
