#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include "microros/actuators.hpp"
#include "actuators.hpp"

#include "microros/error_handling.hpp"


Actuators actuators;

MicroRosActuators* MicroRosActuators::actuators_instance = nullptr;

MicroRosActuators::MicroRosActuators(rcl_node_t* node, rclc_support_t* support, rclc_executor_t* executor): 
      node(node), support(support), executor(executor) {
        actuators_instance = this;
        actuators.setup_led_on_off();
        actuators.setup_buzzer();
        actuators.setup_motor();
        actuators.setup_oled();

        static char oled_buffer[100];

        std_msgs__msg__String__init(&oled_text_);

        oled_text_.data.data = oled_buffer;
        oled_text_.data.size = 0;
        oled_text_.data.capacity = sizeof(oled_buffer);
}

void MicroRosActuators::init_cmd_vel_sub() {
    RCCHECK(rclc_subscription_init_default(
        &cmd_vel_sub_,
        node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "cmd_vel"
    ));
    RCCHECK(rclc_executor_add_subscription(
        executor,
        &cmd_vel_sub_,
        &cmd_vel_,
        &MicroRosActuators::callbackCmdVel,
        ON_NEW_DATA
    ));
}

void MicroRosActuators::init_motors_cmd_sub() {
    RCCHECK(rclc_subscription_init_default(
        &motors_cmd_sub_,
        node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(robot_interfaces, msg, MotorsCmd),
        "motors_cmd"
    ));
    RCCHECK(rclc_executor_add_subscription(
        executor,
        &motors_cmd_sub_,
        &motors_cmd_,
        &MicroRosActuators::callbackMotorsCmd,
        ON_NEW_DATA
    ));
}

void MicroRosActuators::init_led_sub() {
    RCCHECK(rclc_subscription_init_default(
        &led_sub_,
        node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool),
        "led_state"
    ));
    RCCHECK(rclc_executor_add_subscription(
        executor,
        &led_sub_,
        &led_state_,
        &MicroRosActuators::callbackLed,
        ON_NEW_DATA
    ));
}

void MicroRosActuators::init_buzzer_sub() {
    RCCHECK(rclc_subscription_init_default(
        &buzzer_sub_,
        node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool),
        "buzzer_state"
    ));
    RCCHECK(rclc_executor_add_subscription(
        executor,
        &buzzer_sub_,
        &buzzer_state_,
        &MicroRosActuators::callbackBuzzer,
        ON_NEW_DATA
    ));
}

void MicroRosActuators::init_oled_sub() {
    if (USB_DEBUG) {
        Serial.println("INIT OLED SUB");
    }
    RCCHECK(rclc_subscription_init_default(
        &oled_sub_,
        node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String /*String*/),
        "oled_text"
    ));
    RCCHECK(rclc_executor_add_subscription(
        executor,
        &oled_sub_,
        &oled_text_,
        &MicroRosActuators::callbackOled,
        ON_NEW_DATA
    ));
    if (USB_DEBUG) {
        Serial.println("OLED SUB ADDED");
    }
}


void MicroRosActuators::callbackCmdVel(const void * msgin) {
    const geometry_msgs__msg__Twist* msg = (const geometry_msgs__msg__Twist*)msgin;

    if (msg == nullptr) {
        return;
    }

    float linear_velocity = msg->linear.x;    // m/s
    float angular_velocity = msg->angular.z;  // rad/s

    const float sampling_time = 0.1; // seconds
    const float meters_per_step = 0.000126;
    const float steps_per_360_degrees = 4300;

    int linear_steps = (int)((linear_velocity * sampling_time / meters_per_step));
    int angular_steps = (int)((angular_velocity * sampling_time / (2 * M_PI)) * steps_per_360_degrees);


    // forward
    if ((linear_velocity > 0) && (angular_velocity == 0)) {
      actuators.motor_step(1, 1, linear_steps);
      actuators.motor_step(2, 0, linear_steps);
    }

    // backward
    else if ((linear_velocity < 0) && (angular_velocity == 0)) {
      actuators.motor_step(1, 0, linear_steps);
      actuators.motor_step(2, 1, linear_steps);
    }

    // only turn clockwise
    else if ((angular_velocity > 0) && (linear_velocity == 0)) {
      actuators.motor_step(1, 1, angular_steps);
      actuators.motor_step(2, 1, angular_steps);
    }

    // only turn counter-clockwise
    else if ((angular_velocity < 0) && (linear_velocity == 0)) {
      actuators.motor_step(1, 0, angular_steps);
      actuators.motor_step(2, 0, angular_steps);
    }

    // circle forward clockwise = left wheel forward
    else if ((linear_velocity > 0) && (angular_velocity > 0)) {
      actuators.motor_step(1, 1, linear_steps);
      actuators.motor_step(2, 0, 0);
    }

    // circle forward counter-clockwise = right wheel foward
    else if ((linear_velocity > 0) && (angular_velocity < 0)) {
      actuators.motor_step(1, 0, 0);
      actuators.motor_step(2, 1, linear_steps);
    }

    // circle backward counter-clockwise = left wheel backward
    else if ((linear_velocity < 0) && (angular_velocity > 0)) {
      actuators.motor_step(1, 0, linear_steps);
      actuators.motor_step(2, 0, 0);
    }

    // circle backward counter-clockwise = right wheel backward
    else if ((linear_velocity < 0) && (angular_velocity < 0)) {
      actuators.motor_step(1, 0, 0);
      actuators.motor_step(2, 0, linear_steps);
    }

    // else no movement
    else {
      actuators.motor_step(1, 0, 0);
      actuators.motor_step(2, 0, 0);
    }
}

void MicroRosActuators::callbackMotorsCmd(const void * msgin) {
    const robot_interfaces__msg__MotorsCmd* msg = (const robot_interfaces__msg__MotorsCmd*) msgin;
    if (msg != NULL) {
        if (USB_DEBUG) {
            Serial.printf("msg->dir_left = %s, msg->dir_right = %s, msg->steps = %d\n", 
            msg->dir_left ? "true": "false", msg->dir_right ? "true": "false", msg->steps);
        }
        actuators.motor_step(1, msg->dir_left, msg->steps);
        actuators.motor_step(2, msg->dir_right, msg->steps);
    }
}

void MicroRosActuators::callbackLed(const void * msgin) {
    const std_msgs__msg__Bool* msg = (const std_msgs__msg__Bool*) msgin;
    if (msg != NULL) {
        if (USB_DEBUG) {
            Serial.printf("msg->data = %s\n", (msg->data)? "true": "false");
        }
        if (msg->data == true) {
            actuators.led_on();
        } else {
            actuators.led_off();
        }
    }
}

void MicroRosActuators::callbackBuzzer(const void * msgin){
    const std_msgs__msg__Bool* msg = (const std_msgs__msg__Bool*) msgin;
    if (msg != NULL) {
        if (USB_DEBUG) {
            Serial.printf("msg->data = %s\n", (msg->data)? "true": "false");
        }
        if (msg->data == true) {
            actuators.buzzer_on();
        } else {
            actuators.buzzer_off();
        }
    }
}

void MicroRosActuators::callbackOled(const void * msgin){
    const std_msgs__msg__String* msg = (const std_msgs__msg__String*) msgin;
    if (msg != NULL) {
        if (USB_DEBUG) {
            Serial.printf("msg->data = %s\n", msg->data.data);
        }
        actuators.oled_print(String(msg->data.data));
    }
}