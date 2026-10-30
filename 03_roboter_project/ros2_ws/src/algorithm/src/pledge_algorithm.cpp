#include "algorithm/pledge_algorithm.hpp"
#include "algorithm/robot_controller.hpp"



PledgeAlgorithm::PledgeAlgorithm(ControllerNode* controller_node) : controller_node_(controller_node), angular_sum_(0) {
}

void PledgeAlgorithm::run() {
    driveToWall();
    controller_node_->turnClockwise();
    followWall();
}

// void PledgeAlgorithm::stop() {
//     RCLCPP_INFO(controller_node_->get_logger(), "Stopping Pledge Algorithm.");
//     running_ = false;
// }

void PledgeAlgorithm::driveToWall() {
    controller_node_->setOled("Moving forward"); 
    while(controller_node_->getSonarFrontValue() >= 30) {
        controller_node_->moveForward();
    }

    controller_node_->moveForwardExact(controller_node_->getSonarFrontValue() - 10);
}
    
void PledgeAlgorithm::followWall() {
    /*
    TODO: 
    By now it turns directly counter clockwise if there is no wall on the left.
    In the future you prbably have to know that you have driven far enough 
    so you don't drive into a wall directly after the turn.
    */
    while (running_) {

        // error handling for sensors
        if (controller_node_->getSonarFrontValue() == 0 || controller_node_->getSonarLeftValue() == 0) {
            RCLCPP_ERROR(controller_node_->get_logger(), "Sensors most likely not working."); //maybe error topic or OLED display or both, possible that robot in a wall (drive backward???)
        }

        // wall left and enough space in front
        else if (controller_node_->getSonarFrontValue() > 20 && controller_node_->getSonarLeftValue() <= 20) {
            controller_node_->moveForward();
        }

        // no wall left and enough space in front so enogh space to drive forward and then turn counter-clockwise
        else if (controller_node_->getSonarFrontValue() > 20 && controller_node_->getSonarLeftValue() > 20) {
            int last_wall_sample = controller_node_->getLastWallSample();

            controller_node_->moveBeforeLeftTurn(last_wall_sample);
            controller_node_->turnCounterClockwise();
            
            for (int i = 0; i < 3; i++) {
                if (controller_node_->getSonarFrontValue() > 20) {
                    controller_node_->moveForwardExact(10);
                } else {
                    RCLCPP_WARN(controller_node_->get_logger(), "Not enough space to drive forward!!! Please move away, because there was enough space before");
                    while (controller_node_->getSonarLeftValue() <= 15) {
                        rclcpp::sleep_for(std::chrono::milliseconds(200));
                    }
                }
            }
        }

        // // no wall on left side
        // else if (controller_node_->getSonarLeftValue() >= 20) {  
        //     /* condition equals 
        //     ((controller_node_->getSonarLeftValue() > 10 && controller_node_->getSonarFrontValue() > 20) 
        //     || 
        //     (controller_node_->getSonarLeftValue() > 10 && controllerNode.getSonarFrontValue() < 20))
        //     */
        //     controller_node_->moveForward();
        //     if (controller_node_->getSonarLeftValue() >= 20) {
        //         controller_node_->turnCounterClockwise();
        //     }
            
        //     // move forward if there is enough space
        //     if (controller_node_->getSonarFrontValue() >= 20) {
        //         controller_node_->moveForward();
        //     }

        //     // if there is enough space in front and last second-to-last action was a turn, move forward so the wall on the left side is not lost
        //     if ((controller_node_->turn_counter_  > 0) && (controller_node_->getSonarFrontValue() >= 20)) {
        //         controller_node_->moveForward();
        //     }

        //     // if (controller_node_->turn_counter_ == 0) {
        //     //     controller_node_->moveForward();
        //     //     //controller_node_->moveBeforeLeftTurn();
            
        //     // } else {
        //     //     for (int i = 0; i < 2; i++)
        //     //     if (controller_node_->getSonarFrontValue() >= 20) {
        //     //         controller_node_->moveForward();
        //     //     }
        //     // }

        //     // move forward if there is enough space
            

        //     // if there is enough space in front and last second-to-last action was a turn, move forward so the wall on the left side is not lost
        //     // if ((controller_node_->turn_counter_  > 0) && (controller_node_->getSonarFrontValue() >= 20)) {
        //     //     controller_node_->moveForward();
        //     // }
        // }  
        
        // wall left and wall in front
        else if (controller_node_->getSonarFrontValue() <= 20 && controller_node_->getSonarLeftValue() <= 20) {

            RCLCPP_INFO(controller_node_->get_logger(), "controller_node_->getSonarFrontValue() - 10) > 0 ? (controller_node_->getSonarFrontValue() - 10) : 0 => %d",
                (controller_node_->getSonarFrontValue() - 10) > 0 ? (controller_node_->getSonarFrontValue() - 10) : 0);
            controller_node_->moveForwardExact((controller_node_->getSonarFrontValue() - 10) > 0 ? (controller_node_->getSonarFrontValue() - 10) : 0);
            controller_node_->turnClockwise();
            // if (controller_node_->getSonarFrontValue() > 20) {
            //     controller_node_->moveForward();
            // }
        } 
        
        // wall in front and no wall left, but not enough space in front to drive forward and turn counter clockwise
        // so try to turn clockwise and hope that there is enough space
        else if (controller_node_->getSonarFrontValue() <= 20 && controller_node_->getSonarLeftValue() > 20) {
            controller_node_->moveForwardExact(controller_node_->getSonarFrontValue() - 10);
            // controller_node_->turnClockwise();
            controller_node_->turnCounterClockwise();
        }

        // last case
        // else if (controller_node_->getSonarFrontValue() > 20 && controller_node_->getSonarFrontValue() <= 20 && controller_node_->getSonarLeftValue() > 20) {
        //     controller_node_->moveForward();
        // }
        
        // else error
        else {
            RCLCPP_ERROR(controller_node_->get_logger(), "Pledge algorithm: No matching condition found.");
            break;
        }


        // stop/ finish if a bumper is pressed
        if (controller_node_->wasLeftPressed() || controller_node_->wasRightPressed()) {
            break;
        }
    }
    controller_node_->moveBackward();
    controller_node_->setOled("Pledge algorithm finished.");
    RCLCPP_INFO(controller_node_->get_logger(), "Pledge algorithm terminated because the bumpers are pressed.");
}
