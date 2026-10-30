#include <Arduino.h>
#include <NewPing.h>
#include <Wire.h>
#include "mpu6500.h"
#include "microros/error_handling.hpp"


#include "sensors.hpp"

Sensors::Sensors(): sonarSensorFront(SONAR_FRONT_TRIGGER, SONAR_FRONT_ECHO, MAX_DISTANCE), 
    sonarSensorLeft(SONAR_LEFT_TRIGGER, SONAR_LEFT_ECHO, MAX_DISTANCE),
    imu(&Wire, bfs::Mpu6500::I2C_ADDR_PRIM) {

}

void Sensors::setup_bumpers() {
    // bumpers
    pinMode(BUMPER_LEFT, INPUT);
    pinMode(BUMPER_RIGHT, INPUT);
}

bool Sensors::setup_imu(){
    Wire.begin(IMU_SDA, IMU_SCL); // intializes I2C on the correct pins
    Wire.setClock(400000);

    uint8_t whoami = 0;

    Wire.beginTransmission(0x68);
    Wire.write(0x75);           // WHO_AM_I
    Wire.endTransmission(false);

    Wire.requestFrom(0x68, 1);

    if (USB_DEBUG) {
        if (Wire.available()) {
            whoami = Wire.read();
            Serial.print("WHO_AM_I = 0x");
            Serial.println(whoami, HEX);
        } else {
            Serial.println("WHO_AM_I konnte nicht gelesen werden");
        }
    }
    
    bool ok = imu.Begin();
    if (USB_DEBUG) {
        if (!ok) {
            Serial.println("IMU init failed");
        } else {
            Serial.println("IMU init successful");
        }
    }
    
    return ok;
}

void Sensors::scan_i2c(){
    Serial.println("I2C Scan:");
    for(byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);

        byte error = Wire.endTransmission();

        if(error == 0)
        {
            Serial.print("Device found: 0x");
            Serial.println(address, HEX);
        }
    }
    Serial.println("I2C scan finished");
}

bool Sensors::bumper_left_pressed(){
    return !digitalRead(BUMPER_LEFT);
}

bool Sensors::bumper_right_pressed(){
    return !digitalRead(BUMPER_RIGHT);
}

unsigned int Sensors::sonar_front() {
    unsigned int uS = sonarSensorFront.ping();
    return uS / US_ROUNDTRIP_CM;
}

unsigned int Sensors::sonar_left() {
    unsigned int uS = sonarSensorLeft.ping();
    return uS / US_ROUNDTRIP_CM;
}

std::array<float, 6> Sensors::imu_values() {
    std::array<float, 6> imu_data = {0, 0, 0, 0, 0, 0};
    if (imu.Read()) {
        imu_data[0] = imu.accel_x_mps2();
        imu_data[1] = imu.accel_y_mps2();
        imu_data[2] = imu.accel_z_mps2();

        imu_data[3] = imu.gyro_x_radps();
        imu_data[4] = imu.gyro_y_radps();
        imu_data[5] = imu.gyro_z_radps();
    } else {
        Serial.println("IMU just 0 cos initialization was not successful.");
    }
    
    return imu_data;
}
