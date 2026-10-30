#pragma once

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"



class BuzzerPublisher {

public:
    BuzzerPublisher(rclcpp::Node* node);

    void setBuzzer(bool on);

private:
    rclcpp::Node* node_;
    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr buzzer_publisher_;
    
    bool last_state_ = false;
};
