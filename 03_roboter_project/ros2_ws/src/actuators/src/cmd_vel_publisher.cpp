#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include <cmath>



class MotorPublisherNode : public rclcpp::Node {

public:
    MotorPublisherNode() : Node("motor_publisher", "actors") {

        // declare params
        this->declare_parameter<int>("num_sides", 2);
        this->declare_parameter<double>("forward_duration", 3.0);

        // get params
        num_sides_ = this->get_parameter("num_sides").as_int();
        forward_duration_ = this->get_parameter("forward_duration").as_double();

        // initialize state machine
        if (num_sides_ < 3) {
            state_ = State::CIRCLE;
            turn_duration_ = 0.0;
        }

        else {
            state_ = State::FORWARD;
            double turn_angle = 2.0 * M_PI / num_sides_;
            turn_duration_ = turn_angle / angular_vel_;
        }

        // set real start time
        state_start_time_ = this->now().seconds();
        
        // publisher
        motor_publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);

        // timer
        motor_timer_ = this->create_wall_timer(std::chrono::milliseconds(100), std::bind(&MotorPublisherNode::update, this));
        
        RCLCPP_INFO(this->get_logger(), "motor publisher node has been started.");
    }

private:
    enum class State {
        FORWARD,
        TURN,
        CIRCLE
    };

    void update() {
        // track time
        double now = this->now().seconds();
        double elapsed = now - state_start_time_;

        // initialize message
        geometry_msgs::msg::Twist msg;

        // circle
        if (state_ == State::CIRCLE) {
            msg.linear.x = linear_vel_;
            msg.angular.z = angular_vel_;
        }

        // forward
        else if (state_ == State::FORWARD) {
            msg.linear.x = linear_vel_;
            msg.angular.z = 0.0;

            if (elapsed >= forward_duration_) {
                state_ = State::TURN;
                state_start_time_ = now;
            }
        }

        // turn
        else if (state_ == State::TURN) {
            msg.linear.x = 0.0;
            msg.angular.z = angular_vel_;

            if (elapsed >= turn_duration_) {
                state_ = State::FORWARD;
                state_start_time_ = now;
            }
        }

        // publish
        motor_publisher_->publish(msg);

        RCLCPP_INFO(
            this->get_logger(),
            "Publishing: linear.x=%.2f, angular.z=%.2f",
            msg.linear.x,
            msg.angular.z
        );
    }

    // params
    int num_sides_;
    double forward_duration_;

    // define robot constants
    const double linear_vel_ = 0.0416;
    const double angular_vel_ = 0.15 * M_PI;

    // states
    State state_;
    double state_start_time_;
    double turn_duration_;

    // ROS2
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr motor_publisher_;
    rclcpp::TimerBase::SharedPtr motor_timer_;
};



int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<MotorPublisherNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
