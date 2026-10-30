#pragma once

#include <array>
#include <NewPing.h>
#include "mpu6500.h"

class Sensors{
public:
    Sensors();

    void setup_bumpers();
    bool setup_imu();

    bool bumper_left_pressed();
    bool bumper_right_pressed();

    unsigned int sonar_front();
    unsigned int sonar_left();
    int color(); // color sensor, what value range (bits)

    std::array<float, 6> imu_values();

    void scan_i2c();

private:
    static constexpr int IMU_SDA = 21;
    static constexpr int IMU_SCL = 22;
    static constexpr int IMU_INT = 34;

    
    static constexpr int SONAR_FRONT_ECHO = 25;
    static constexpr int SONAR_FRONT_TRIGGER = 26;
    static constexpr int SONAR_LEFT_ECHO = 23;
    static constexpr int SONAR_LEFT_TRIGGER = 27;
    static constexpr uint16_t MAX_DISTANCE = 200;

    static constexpr int APDS_SDA = 21;
    static constexpr int APDS_SCL = 22;

    static constexpr int BUMPER_RIGHT = 12;
    static constexpr int BUMPER_LEFT = 14;

    NewPing sonarSensorFront;
    NewPing sonarSensorLeft;
    bfs::Mpu6500 imu;
};