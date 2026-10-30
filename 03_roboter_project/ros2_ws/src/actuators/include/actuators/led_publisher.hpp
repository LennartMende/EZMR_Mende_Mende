#pragma once

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"



class LedPublisher {

public:
    LedPublisher(rclcpp::Node* node);

    void setLed(bool on);

private:
    rclcpp::Node* node_;
    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr led_publisher_;
    
    bool last_state_ = false;
};
