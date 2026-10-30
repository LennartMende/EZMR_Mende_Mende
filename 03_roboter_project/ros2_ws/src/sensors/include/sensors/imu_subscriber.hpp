#pragma once

#include "rclcpp/rclcpp.hpp"
#include "robot_interfaces/msg/imu_data.hpp"


struct ImuValues {
    float ax;
    float ay;
    float az;

    float wx;
    float wy;
    float wz;
};


class ImuSubscriber {

public:
    ImuSubscriber(rclcpp::Node* node);
    ImuValues getImuValues();

private:
    void callbackImu(const robot_interfaces::msg::ImuData::SharedPtr msg);
    rclcpp::Node* node_;
    ImuValues imu_values_;
    rclcpp::Subscription<robot_interfaces::msg::ImuData>::SharedPtr imu_subscriber_;
};