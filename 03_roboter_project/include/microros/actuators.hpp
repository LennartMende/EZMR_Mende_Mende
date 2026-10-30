#pragma once

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <geometry_msgs/msg/twist.h>
#include <std_msgs/msg/bool.h>
#include <std_msgs/msg/string.h>

#include "robot_interfaces/msg/motors_cmd.h"
#include "robot_interfaces/srv/set_actuators.h"
#include "microros/error_handling.hpp"

#include "secrets.hpp"


class MicroRosActuators {

public:

    MicroRosActuators(rcl_node_t* node, rclc_support_t* support, rclc_executor_t* executor);

    void init_cmd_vel_sub();
    void init_motors_cmd_sub();
    void init_led_sub();
    void init_buzzer_sub();
    void init_oled_sub();
    void init_set_oled_service();

private:

    static MicroRosActuators* actuators_instance;

    rcl_node_t* node;
    rclc_support_t* support;
    rclc_executor_t* executor;

    // subscribers
    rcl_subscription_t cmd_vel_sub_;
    geometry_msgs__msg__Twist cmd_vel_;

    rcl_subscription_t motors_cmd_sub_;
    robot_interfaces__msg__MotorsCmd motors_cmd_;

    rcl_subscription_t led_sub_;
    std_msgs__msg__Bool led_state_;

    rcl_subscription_t buzzer_sub_;
    std_msgs__msg__Bool buzzer_state_;

    rcl_subscription_t oled_sub_;
    // std_msgs__msg__String oled_text_;
    std_msgs__msg__String oled_text_;


    // do we need the following???
    // void processCmdVel(const geometry_msgs__msg__Twist* msg);

    // static void callbackCmdVel(const void* msgin);


    // rcl_service_t set_actuators_service_;
    // robot_interfaces__srv__SetActuators_Request actuators_request_;
    // robot_interfaces__srv__SetActuators_Response actuators_response_;

    // static void callbackSetActuators(const void* request_msg, void* response_msg);

    // void processSetActuators(const robot_interfaces__srv__SetActuators_Request* request, robot_interfaces__srv__SetActuators_Response* response);



    // void subscribeCmdVel();
    // void subscribeLed();
    // void subscribeOled();

    static void callbackCmdVel(const void * msgin);
    static void callbackMotorsCmd(const void * msgin);
    static void callbackLed(const void * msgin);
    static void callbackBuzzer(const void * msgin);
    static void callbackOled(const void * msgin);
};
