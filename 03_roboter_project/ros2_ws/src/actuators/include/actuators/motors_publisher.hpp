#pragma once

#include "rclcpp/rclcpp.hpp"
#include "robot_interfaces/msg/motors_cmd.hpp"
#include "std_msgs/msg/bool.hpp"



class MotorsPublisher {

public:
    MotorsPublisher(rclcpp::Node* node);

    void setMotion(bool dir_left, bool dir_right, uint16_t steps);


private:
    rclcpp::Node* node_;
    rclcpp::Publisher<robot_interfaces::msg::MotorsCmd>::SharedPtr motors_publisher_;

    bool last_dir_left_ = false;
    bool last_dir_right_ = false;
    uint16_t last_steps_ = 0;
};
