#include "sensors/sonar_front_subscriber.hpp"

using namespace std::placeholders;



SonarFrontSubscriber::SonarFrontSubscriber(rclcpp::Node* node) : node_(node) {
    sonar_front_subscriber_ = node_->create_subscription<std_msgs::msg::Int16>(
            "sonar_front", 10, std::bind(&SonarFrontSubscriber::callbackSonarFront, this, _1));

    RCLCPP_INFO(node_->get_logger(), "Sonar front subscriber node has been started.");
}

// getter for sonar front value
int16_t SonarFrontSubscriber::getDistance() {
    return sonar_front_value_;
}

void SonarFrontSubscriber::callbackSonarFront(const std_msgs::msg::Int16::SharedPtr msg) {
    if (msg->data != 0) {
        sonar_front_value_ = msg->data;
    }
    // else {
    //     RCLCPP_ERROR(node_->get_logger(), "Sonar front value = 0. Sensor most likely not working.");
    // }
    // RCLCPP_INFO(node_->get_logger(), "Sonar front value = %d", msg->data);
}
