#pragma once

#include "rclcpp/rclcpp.hpp"

// #include "actuators/actuators_client.hpp"

#include "sensors/bumpers_subscriber.hpp"
#include "sensors/imu_subscriber.hpp"
#include "sensors/sonar_front_subscriber.hpp"
#include "sensors/sonar_left_subscriber.hpp"

#include "actuators/motors_publisher.hpp"
#include "actuators/led_publisher.hpp"
#include "actuators/buzzer_publisher.hpp"
#include "actuators/oled_publisher.hpp"



class ControllerNode : public rclcpp::Node {
    friend class PledgeAlgorithm;
public:
    ControllerNode();

    int16_t getSonarFrontValue();
    int16_t getSonarLeftValue();
    int getLastWallSample();
    //int resetLastWallSample();
    bool isLeftPressed();
    bool isRightPressed();
    bool wasLeftPressed();
    bool wasRightPressed();
    ImuValues getImuValues();

    // void sendActuatorsRequest(
    //     bool buzzer_on,
    //     bool led_on,
    //     const std::string &motion_command,
    //     uint16_t distance_cm,
    //     const std::string &info_text
    // );

    void setOled(const std::string &text);

    void handleBuzzerAndLedDuringTurn(rclcpp::Time start_time);
    void moveForwardExact(uint16_t distance_cm);
    void moveBeforeLeftTurn(int last_wall_sample);
    void moveForward();
    void moveBackward();
    void turnClockwise();
    void turnCounterClockwise();

private:
    int turn_counter_ = 0; // 0 if last 2 actions were not turns, otherwise 1 if second-to-last action was a turn, 2 if last action was a turn
    // std::shared_ptr<ActuatorsClient> actuators_client_;

    std::shared_ptr<BumpersSubscriber> bumpers_subscriber_;
    std::shared_ptr<ImuSubscriber> imu_subscriber_;
    std::shared_ptr<SonarFrontSubscriber> sonar_front_subscriber_;
    std::shared_ptr<SonarLeftSubscriber> sonar_left_subscriber_;

    std::shared_ptr<MotorsPublisher> motors_publisher_;
    std::shared_ptr<LedPublisher> led_publisher_;
    std::shared_ptr<BuzzerPublisher> buzzer_publisher_;
    std::shared_ptr<OledPublisher> oled_publisher_;

    void setBuzzer(bool on);
    void setLed(bool on);
    void setMotorsCmd(bool dir_left, bool dir_right, uint16_t steps);
};
