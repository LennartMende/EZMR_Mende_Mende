#pragma once

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"



class OledPublisher {

public:
    OledPublisher(rclcpp::Node* node);

    void setText(const std::string &text);

private:
    rclcpp::Node* node_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr oled_publisher_;
    
    std::string last_text_;
};
