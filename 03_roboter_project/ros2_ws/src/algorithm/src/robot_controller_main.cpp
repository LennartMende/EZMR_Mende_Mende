#include "algorithm/robot_controller.hpp"
#include "algorithm/pledge_algorithm.hpp"

#include <thread>
#include <csignal>


void signalHandler(int signal);

// std::shared_ptr<ControllerNode> node;
// std::unique_ptr<PledgeAlgorithm> pledge_algorithm;

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ControllerNode>();

    std::thread spin_thread([&]() {rclcpp::spin(node);});

    PledgeAlgorithm pledge_algorithm(node.get());
    // std::signal(SIGINT, signalHandler);
    pledge_algorithm.run();

    rclcpp::shutdown();
    spin_thread.join();
    return 0;
}


// void signalHandler(int signal)
// {
//     if (signal == SIGINT) {
//         RCLCPP_INFO(
//             rclcpp::get_logger("main"),
//             "STRG+C pressed. Shutting down ROS2 node and stopping pledge algorithm."
//         );

//         if (pledge_algorithm != nullptr) {
//             pledge_algorithm->stop();
//         }
//     }
// }