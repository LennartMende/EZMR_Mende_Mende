#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <micro_ros_platformio.h>

#include "secrets.hpp"

#include "microros/microros.hpp"
#include "microros/error_handling.hpp"

MicroRos* microRos = nullptr;

void setup() {
    Serial.begin(115200);
    Serial.println("NEW FIRMWARE STARTED");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }

    Serial.println("WiFi connected");
    Serial.write("The ESP's ID is:");
    Serial.println(WiFi.localIP());


    ArduinoOTA.setHostname("ESP32-Robo");
    ArduinoOTA.begin();


    IPAddress agent_ip(AGENT_IP);

    if (USB_DEBUG) {
        Serial.println("Before microros Wifi transport");
    }

    set_microros_wifi_transports(
        const_cast<char*>(WIFI_SSID),
        const_cast<char*>(WIFI_PASSWORD),
        agent_ip,
        AGENT_PORT
    );

    if (USB_DEBUG) {
        Serial.println("After microros Wifi transport");
    }


    // DAS HAT GEFEHLT:
    microRos = new MicroRos();


    microRos->init();
    if (USB_DEBUG) {
        Serial.println("microRos setup completed");
    }
    
}

void loop() {

    ArduinoOTA.handle();
    
    // if (USB_DEBUG) {
    //     Serial.println("After OTA handle");
    // }

    // if (microRos != nullptr) {
    //     RCSOFTCHECK(
            rclc_executor_spin_some(
                &microRos->executor,
                RCL_MS_TO_NS(100)
            );
    //     );
    // }
}
