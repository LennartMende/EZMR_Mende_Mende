#include "algorithm/robot_controller.hpp"



class PledgeAlgorithm {

public:
    PledgeAlgorithm(ControllerNode* controller_node);
    void run();
    // void stop();

private:
    ControllerNode* controller_node_;
    int angular_sum_;
    bool running_ = true;
    int turn_counter_ = 0;

    void driveToWall();
    void followWall();
};
