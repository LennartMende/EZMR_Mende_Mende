#pragma once
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int16.h>

#include "robot_interfaces/msg/bumpers_states.h"
#include "robot_interfaces/msg/imu_data.h"

#include "sensors.hpp"
#include "actuators.hpp"


class MicroRosSensors {
public:
    MicroRosSensors(rcl_node_t* node, rclc_support_t* support, rclc_executor_t* executor);

    void init_sonar_front_pub();
    void init_sonar_left_pub();
    void init_bumpers_pub();
    void init_imu_pub();

private:
    static MicroRosSensors* sensors_instance;

    rcl_node_t* node;
    rclc_support_t* support;
    rclc_executor_t* executor;

    rcl_publisher_t sonar_front_pub_;
    rcl_timer_t sonar_front_timer_;
    static constexpr uint8_t sonar_front_timer_timeout = 200;
    std_msgs__msg__Int16 sonar_front;

    rcl_publisher_t sonar_left_pub_;
    rcl_timer_t sonar_left_timer_;
    static constexpr uint8_t sonar_left_timer_timeout = 200;
    std_msgs__msg__Int16 sonar_left;

    rcl_publisher_t bumpers_pub_;
    rcl_timer_t bumpers_timer_;
    static constexpr uint8_t bumpers_timer_timeout = 200;
    robot_interfaces__msg__BumpersStates bumpers; // add interface for microros!!!

    rcl_publisher_t imu_pub_;
    rcl_timer_t imu_timer_;
    static constexpr uint8_t imu_timer_timeout = 200;
    robot_interfaces__msg__ImuData imu; // add interface for microros!!!

    void publishSonarFront();
    void publishSonarLeft();
    void publishBumpers();
    void publishImu();

    static void callbackSonarFront(rcl_timer_t *timer, int64_t last_call_time);
    static void callbackSonarLeft(rcl_timer_t *timer, int64_t last_call_time);
    static void callbackBumpers(rcl_timer_t *timer, int64_t last_call_time);
    static void callbackImu(rcl_timer_t *timer, int64_t last_call_time);
};