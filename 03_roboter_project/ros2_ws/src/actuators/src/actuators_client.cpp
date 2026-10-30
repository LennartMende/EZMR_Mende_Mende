// #include "actuators/actuators_client.hpp"



// ActuatorsClient::ActuatorsClient(rclcpp::Node* node) {
    
//     // create client
//     actuators_client_ = node->create_client<robot_interfaces::srv::SetActuators>("set_actuators");


//     // wait 5 seconds for service to be available
//     if (!actuators_client_->wait_for_service(std::chrono::seconds(5))) {
//         RCLCPP_WARN(node->get_logger(), "Service not available!");
//     }
// }


// void ActuatorsClient::send_request(
//     bool buzzer,
//     bool led,
//     std::string motion,
//     uint16_t distance,
//     std::string text) {

//     // create request
//     auto req = std::make_shared<robot_interfaces::srv::SetActuators::Request>();

//     req->buzzer_on = buzzer;
//     req->led_on = led;
//     req->motion_command = motion;
//     req->distance_cm = distance;
//     req->info_text = text;

//     // async call
//     actuators_client_->async_send_request(req);
// }
