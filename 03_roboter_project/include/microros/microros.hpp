#pragma once
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include "microros/sensors.hpp"
#include "microros/actuators.hpp"

class MicroRos {
public:
    MicroRos();
    void init();

    rclc_executor_t executor;

private:
    rclc_support_t support;
    rcl_allocator_t allocator;
    rcl_node_t esp_node;

    MicroRosSensors* microRosSensors;
    MicroRosActuators* microRosActuators;
};