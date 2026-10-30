#include "microros/error_handling.hpp"
#include <ArduinoOTA.h>

void error_loop() {
  if (USB_DEBUG) {
    Serial.println("ERROR LOOP");
  }

  while (1) {
    if (USB_DEBUG) {
      Serial.println("ROS INIT FAILED");
    }
    ArduinoOTA.handle();
  }
}
