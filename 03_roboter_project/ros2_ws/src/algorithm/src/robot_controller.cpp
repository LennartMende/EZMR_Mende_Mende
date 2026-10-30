#include "algorithm/robot_controller.hpp"
#include "rclcpp/rclcpp.hpp"



ControllerNode::ControllerNode() : Node("controller_node") {

    // create actuators client
    // actuators_client_ = std::make_shared<ActuatorsClient>(this);

    // create actors publishers
    motors_publisher_ = std::make_shared<MotorsPublisher>(this);
    led_publisher_ = std::make_shared<LedPublisher>(this);
    buzzer_publisher_ = std::make_shared<BuzzerPublisher>(this);
    oled_publisher_ = std::make_shared<OledPublisher>(this);

    // create sensor subscribers
    bumpers_subscriber_ = std::make_shared<BumpersSubscriber>(this);
    imu_subscriber_ = std::make_shared<ImuSubscriber>(this);
    sonar_front_subscriber_ = std::make_shared<SonarFrontSubscriber>(this);
    sonar_left_subscriber_ = std::make_shared<SonarLeftSubscriber>(this);

    setLed(false);
    setBuzzer(false);
    setOled("Controller Node started.");

    RCLCPP_INFO(this->get_logger(), "Controller Node has been started.");
}

// getters for sensors
int16_t ControllerNode::getSonarFrontValue() {
    return sonar_front_subscriber_->getDistance();
}

int16_t ControllerNode::getSonarLeftValue() {
    return sonar_left_subscriber_->getDistance();
}

int ControllerNode::getLastWallSample() {
    return sonar_left_subscriber_->getLastWallSample();
}

bool ControllerNode::isLeftPressed() {
    return bumpers_subscriber_->isLeftPressed();
}

bool ControllerNode::isRightPressed() {
    return bumpers_subscriber_->isRightPressed();
}

bool ControllerNode::wasLeftPressed() {
    return bumpers_subscriber_->wasLeftPressed();
}

bool ControllerNode::wasRightPressed() {
    return bumpers_subscriber_->wasRightPressed();
}

ImuValues ControllerNode::getImuValues() {
    return imu_subscriber_->getImuValues();
}


// setters for actors
void ControllerNode::setOled(const std::string &text) {
    oled_publisher_->setText(text);
}

void ControllerNode::moveForwardExact(uint16_t distance_cm) {
    oled_publisher_->setText("Moving forward");  
    auto start_time = this->get_clock()->now();
    uint16_t steps = distance_cm * 79.365;
    setMotorsCmd(true, false, steps);
    double wait_time = (static_cast<double>(distance_cm) / 3.5) < 8 ? (static_cast<double>(distance_cm) / 3.0) : 8;
    RCLCPP_INFO(this->get_logger(), "Moving forward exact %d cm, waiting for %.2f seconds.", distance_cm, wait_time);
    // while (true) {
    //     rclcpp::sleep_for(std::chrono::milliseconds(200));
    // }
    while ((this->get_clock()->now() - start_time).seconds() < wait_time) {
        RCLCPP_INFO(this->get_logger(), "Moving forward exact %d cm, waiting for %.2f seconds.", distance_cm, wait_time);
        rclcpp::sleep_for(std::chrono::milliseconds(200));
    }
    if (turn_counter_ > 0) {
        turn_counter_--;
    }
}

void ControllerNode::moveBeforeLeftTurn(int last_wall_sample) {
    double distance =
        15.0 - (last_wall_sample * 0.8 + 1.2);

    if (distance < 0.0) {
        distance = 0.0;
    }

    RCLCPP_INFO(
        this->get_logger(),
        "moveBeforeLeftTurn: last_wall_sample=%d, distance=%.2f",
        getLastWallSample(),
        distance
    );

    moveForwardExact(static_cast<uint16_t>(distance));
}

void ControllerNode::moveForward() {
    oled_publisher_->setText("Moving forward"); 
    auto start_time = this->get_clock()->now();
    setMotorsCmd(true, false, 793);
    while ((this->get_clock()->now() - start_time).seconds() < 2.5) {
        rclcpp::sleep_for(std::chrono::milliseconds(200));
    }
    if (turn_counter_ > 0) {
        turn_counter_--;
    }
}

void ControllerNode::moveBackward() {
    oled_publisher_->setText("Moving backward"); 
    auto start_time = this->get_clock()->now();
    setMotorsCmd(false, true, 793);
    while ((this->get_clock()->now() - start_time).seconds() < 2.5) {
        rclcpp::sleep_for(std::chrono::milliseconds(200));
    }
    if (turn_counter_ > 0) {
        turn_counter_--;
    }
}

void ControllerNode::handleBuzzerAndLedDuringTurn(rclcpp::Time start_time) {

    bool actual_buzzer_state = false;
    bool actual_led_state = false;

    while ((this->get_clock()->now() - start_time).seconds() < 3.5) {
        actual_buzzer_state = !actual_buzzer_state;
        actual_led_state = !actual_led_state;
        this->setBuzzer(actual_buzzer_state);
        this->setLed(actual_led_state);
        rclcpp::sleep_for(std::chrono::milliseconds(200));
    }

    this->setBuzzer(false);
    this->setLed(false);
}

void ControllerNode::turnClockwise() {
    oled_publisher_->setText("Turning clockwise");
    auto start_time = this->get_clock()->now();
    setMotorsCmd(true, true, 1075);
    handleBuzzerAndLedDuringTurn(start_time);
    turn_counter_ = 2;
}

void ControllerNode::turnCounterClockwise() {
    oled_publisher_->setText("Turning counterclockwise"); 
    auto start_time = this->get_clock()->now();
    setMotorsCmd(false, false, 1075);
    handleBuzzerAndLedDuringTurn(start_time);
    turn_counter_ = 2;
}


void ControllerNode::setBuzzer(bool on) {
    buzzer_publisher_->setBuzzer(on);
}

void ControllerNode::setLed(bool on) {
    led_publisher_->setLed(on);
}

void ControllerNode::setMotorsCmd(bool dir_left, bool dir_right, uint16_t steps) {
    motors_publisher_->setMotion(dir_left, dir_right, steps);
}
