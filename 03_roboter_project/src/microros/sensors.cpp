#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include "microros/sensors.hpp"
#include "sensors.hpp"

#include "microros/error_handling.hpp"

Sensors sensors;

MicroRosSensors* MicroRosSensors::sensors_instance = nullptr;

MicroRosSensors::MicroRosSensors(
    rcl_node_t* node, rclc_support_t* support, rclc_executor_t* executor): 
      node(node), support(support), executor(executor) {
        sensors_instance = this;
        sensors.setup_bumpers();
        sensors.setup_imu();
}


// inits
void MicroRosSensors::init_sonar_front_pub() {
  RCCHECK(rclc_publisher_init_default(
    &sonar_front_pub_,
    node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int16),
    "sonar_front"
  ));
  RCCHECK(rclc_timer_init_default(
    &sonar_front_timer_,
    support,
    RCL_MS_TO_NS(sonar_front_timer_timeout),
    callbackSonarFront
  ));
  rclc_executor_add_timer(
    executor,
    &sonar_front_timer_);
}

void MicroRosSensors::init_sonar_left_pub() {
  RCCHECK(rclc_publisher_init_default(
    &sonar_left_pub_,
    node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int16),
    "sonar_left"
  ));
  RCCHECK(rclc_timer_init_default(
    &sonar_left_timer_,
    support,
    RCL_MS_TO_NS(sonar_left_timer_timeout),
    callbackSonarLeft
  ));
  rclc_executor_add_timer(
    executor,
    &sonar_left_timer_);
}

void MicroRosSensors::init_bumpers_pub(){
  RCCHECK(rclc_publisher_init_default(
    &bumpers_pub_,
    node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(robot_interfaces, msg, BumpersStates),
    "bumpers"
  ));
  RCCHECK(rclc_timer_init_default(
    &bumpers_timer_,
    support,
    RCL_MS_TO_NS(bumpers_timer_timeout),
    callbackBumpers
  ));
  rclc_executor_add_timer(
    executor,
    &bumpers_timer_);
}


// publisher methods
void MicroRosSensors::init_imu_pub(){
  RCCHECK(rclc_publisher_init_default(
    &imu_pub_,
    node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(robot_interfaces, msg, ImuData),
    "imu"
  ));
  RCCHECK(rclc_timer_init_default(
    &imu_timer_,
    support,
    RCL_MS_TO_NS(imu_timer_timeout),
    callbackImu
  ));
  rclc_executor_add_timer(
    executor,
    &imu_timer_);
}

void MicroRosSensors::publishSonarFront() {
  sonar_front.data = sensors.sonar_front();
  RCSOFTCHECK(rcl_publish(&sonar_front_pub_, &sonar_front, NULL));
}

void MicroRosSensors::publishSonarLeft() {
  sonar_left.data = sensors.sonar_left();
  RCSOFTCHECK(rcl_publish(&sonar_left_pub_, &sonar_left, NULL));
}

void MicroRosSensors::publishBumpers() {
  robot_interfaces__msg__BumpersStates bumpers;
  bumpers.left_bumper = sensors.bumper_left_pressed();
  bumpers.right_bumper = sensors.bumper_right_pressed();
  RCSOFTCHECK(rcl_publish(&bumpers_pub_, &bumpers, NULL));
}

void MicroRosSensors::publishImu() {
  robot_interfaces__msg__ImuData imu;
  auto imu_data = sensors.imu_values();

  imu.accel_x_mps2 = imu_data[0];
  imu.accel_y_mps2 = imu_data[1];
  imu.accel_z_mps2 = imu_data[2];

  imu.gyro_x_radps = imu_data[3];
  imu.gyro_y_radps = imu_data[4];
  imu.gyro_z_radps = imu_data[5];

  RCSOFTCHECK(rcl_publish(&imu_pub_, &imu, NULL));
}


// callbacks
void MicroRosSensors::callbackSonarFront(rcl_timer_t *timer, int64_t last_call_time){
  RCLC_UNUSED(timer);
  RCLC_UNUSED(last_call_time);
  if ((sensors_instance == nullptr) && (USB_DEBUG)) {
    Serial.println("ERROR: sensors_instance NULL");
    return;
  }
  sensors_instance->publishSonarFront();
}

void MicroRosSensors::callbackSonarLeft(rcl_timer_t *timer, int64_t last_call_time){
  RCLC_UNUSED(timer);
  RCLC_UNUSED(last_call_time);
  sensors_instance->publishSonarLeft();
}

void MicroRosSensors::callbackBumpers(rcl_timer_t *timer, int64_t last_call_time){
  RCLC_UNUSED(timer);
  RCLC_UNUSED(last_call_time);
  sensors_instance->publishBumpers();
}

void MicroRosSensors::callbackImu(rcl_timer_t *timer, int64_t last_call_time){
  RCLC_UNUSED(timer);
  RCLC_UNUSED(last_call_time);
  sensors_instance->publishImu();
}