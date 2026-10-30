#include "sensors/sonar_left_subscriber.hpp"

using namespace std::placeholders;



SonarLeftSubscriber::SonarLeftSubscriber(rclcpp::Node* node) : node_(node) {
    sonar_left_subscriber_ = node_->create_subscription<std_msgs::msg::Int16>(
            "sonar_left", 10, std::bind(&SonarLeftSubscriber::callbackSonarLeft, this, _1));

    RCLCPP_INFO(node_->get_logger(), "Sonar left subscriber node has been started.");
}

// getters
int16_t SonarLeftSubscriber::getDistance() {
    return sonar_left_value_;
}

int SonarLeftSubscriber::getLastWallSample() {
    return last_two_wall_samples_;
}

void SonarLeftSubscriber::callbackSonarLeft(const std_msgs::msg::Int16::SharedPtr msg) {
    if (msg->data != 0) {
        sonar_left_value_ = msg->data;
    }
    // else {
    //     RCLCPP_ERROR(node_->get_logger(), "Sonar left value = 0. Sensor most likely not working.");
    // }

    if (sonar_left_value_ > 40) {
        last_wall_sample_++;
    } else { // means else if (sonar_left_value_ <= 40)
        if (last_wall_sample_ == 0) {
            last_two_wall_samples_ = 0;
        }
        last_wall_sample_ = 0;
    }

    // RCLCPP_INFO(node_->get_logger(), "Sonar left value = %d", msg->data);
}
