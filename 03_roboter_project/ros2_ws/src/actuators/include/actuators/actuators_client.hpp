#pragma once

#include "rclcpp/rclcpp.hpp"
#include "robot_interfaces/srv/set_actuators.hpp"



class ActuatorsClient {

public:
    ActuatorsClient(rclcpp::Node* node);

    void send_request(
        bool buzzer,
        bool led,
        std::string motion,
        uint16_t distance,
        std::string text
    );

private:
    rclcpp::Client<robot_interfaces::srv::SetActuators>::SharedPtr actuators_client_;
};
