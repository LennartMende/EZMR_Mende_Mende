#pragma once

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int16.hpp"

class SonarFrontSubscriber {

public:
    SonarFrontSubscriber(rclcpp::Node* node);
    int16_t getDistance();

private:
    void callbackSonarFront(const std_msgs::msg::Int16::SharedPtr msg);
    rclcpp::Node* node_;
    int16_t sonar_front_value_ = 100;
    rclcpp::Subscription<std_msgs::msg::Int16>::SharedPtr sonar_front_subscriber_;
};
