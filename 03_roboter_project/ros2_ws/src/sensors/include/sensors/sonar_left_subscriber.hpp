#pragma once

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int16.hpp"

class SonarLeftSubscriber {

public:
    SonarLeftSubscriber(rclcpp::Node* node);
    int16_t getDistance();
    int getLastWallSample();

private:
    void callbackSonarLeft(const std_msgs::msg::Int16::SharedPtr msg);
    rclcpp::Node* node_;
    int16_t sonar_left_value_ = 100;
    int last_wall_sample_ = 0; // 0 if there was no change, e. g. 5, if 5 samples ago the distance was the first time > 30
    int last_two_wall_samples_ = 0;
    rclcpp::Subscription<std_msgs::msg::Int16>::SharedPtr sonar_left_subscriber_;
};
