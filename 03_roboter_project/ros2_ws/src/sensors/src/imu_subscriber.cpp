#include "sensors/imu_subscriber.hpp"

using namespace std::placeholders;



ImuSubscriber::ImuSubscriber(rclcpp::Node* node) : node_(node) {
    imu_subscriber_ = node_->create_subscription<robot_interfaces::msg::ImuData>(
            "imu", 10, std::bind(&ImuSubscriber::callbackImu, this, _1));

    RCLCPP_INFO(node_->get_logger(), "Imu subscriber node has been started.");
}

// getter for accel and gyro data
ImuValues ImuSubscriber::getImuValues() {
    return imu_values_;
}


void ImuSubscriber::callbackImu(const robot_interfaces::msg::ImuData::SharedPtr msg) {
    imu_values_.ax = msg->accel_x_mps2;
    imu_values_.ay = msg->accel_y_mps2;
    imu_values_.az = msg->accel_z_mps2;
    imu_values_.wx = msg->gyro_x_radps;
    imu_values_.wy = msg->gyro_y_radps;
    imu_values_.wz = msg->gyro_z_radps;
    // RCLCPP_INFO(node_->get_logger(), "accel_x_mps2 = %f", msg->accel_x_mps2);
    // RCLCPP_INFO(node_->get_logger(), "accel_y_mps2 = %f", msg->accel_y_mps2);
    // RCLCPP_INFO(node_->get_logger(), "accel_z_mps2 = %f", msg->accel_z_mps2);
    // RCLCPP_INFO(node_->get_logger(), "gyro_x_radps = %f", msg->gyro_x_radps);
    // RCLCPP_INFO(node_->get_logger(), "gyro_y_radps = %f", msg->gyro_y_radps);
    // RCLCPP_INFO(node_->get_logger(), "gyro_z_radps = %f", msg->gyro_z_radps);
}
