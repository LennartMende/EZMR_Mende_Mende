#include "microros/microros.hpp"
#include "microros/sensors.hpp"
#include "microros/actuators.hpp"
#include "microros/error_handling.hpp"

MicroRos::MicroRos() {

    if (USB_DEBUG) {
        Serial.println("In MicrosRos Constructor");
    }

    allocator = rcl_get_default_allocator();
    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

    if (USB_DEBUG) {
        Serial.println("Before executor init");
    }

    RCCHECK(rclc_node_init_default(
        &esp_node,
        "esp_node",
        "",
        &support
    ));

    if (USB_DEBUG) {
        Serial.println("After esp_node init");
    }


    // RCCHECK(rclc_node_init_default(
    //     &esp_sensor_node,
    //     "esp_sensor_node",
    //     "esp_sensor",
    //     &support
    // ));

    // if (USB_DEBUG) {
    //     Serial.println("After esp_sensor_node init");
    // }

    // RCCHECK(rclc_node_init_default(
    //     &esp_actuator_node,
    //     "esp_actuator_node",
    //     "esp_actuator",
    //     &support
    // ));

    // if (USB_DEBUG) {
    //     Serial.println("After esp_actuator_node init");
    // }

    RCCHECK(rclc_executor_init(&executor, &support.context, 20, &allocator));

    microRosSensors = new MicroRosSensors(&esp_node, &support, &executor);
    microRosActuators = new MicroRosActuators(&esp_node, &support, &executor);
}

void MicroRos::init() {

    if (USB_DEBUG) {
        Serial.println("Before sonar front init");
    }

    microRosSensors->init_sonar_front_pub();

    if (USB_DEBUG) {
        Serial.println("Before sonar left init");
    }

    microRosSensors->init_sonar_left_pub();

    if (USB_DEBUG) {
        Serial.println("Before bumper init");
    }

    microRosSensors->init_bumpers_pub();

    if (USB_DEBUG) {
        Serial.println("Before IMU init");
    }

    microRosSensors->init_imu_pub();

    if (USB_DEBUG) {
        Serial.println("Before set_actuators_service");
    }

    //microRosActuators->init_set_actuators_service();

    if (USB_DEBUG) {
        Serial.println("Before cmd_vel init");
    }

    microRosActuators->init_cmd_vel_sub();

    if (USB_DEBUG) {
        Serial.println("After cmd_vel init");
    }

    microRosActuators->init_motors_cmd_sub();

    if (USB_DEBUG) {
        Serial.println("After motors_cmd init");
    }

    microRosActuators->init_led_sub();

    if (USB_DEBUG) {
        Serial.println("After led init");
    }

    microRosActuators->init_buzzer_sub();

    if (USB_DEBUG) {
        Serial.println("After buzzer init");
    }

    microRosActuators->init_oled_sub();

    if (USB_DEBUG) {
        Serial.println("After oled init");
    }
}
