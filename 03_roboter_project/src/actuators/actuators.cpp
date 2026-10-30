#include <Arduino.h>

#include "actuators.hpp"
#include "microros/error_handling.hpp"


// setups

// buzzer
void Actuators::setup_buzzer() {
    ledcSetup(BUZZER_CHANNEL, BUZZER_FREQUENCY, 8); // 8 bit resolution, e. g. 10 or 12 bits also possible
    ledcAttachPin(BUZZER, BUZZER_CHANNEL);
}

// LED PWM
void Actuators::setup_led_pwm() {
    ledcSetup(LED_CHANNEL, 5000, 8);
    ledcAttachPin(LED, LED_CHANNEL);
}

// LED on/off
void Actuators::setup_led_on_off() {
    pinMode(LED, OUTPUT);
}

// motor
void Actuators::setup_motor() {
    Serial2.begin(9600, SERIAL_8N1, -1, MOTOR);
}

// OLED
void Actuators::setup_oled() {
    Wire.begin(OLED_SDA, OLED_SCL);

    if (USB_DEBUG) {
        if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
            Serial.println("OLED not found");
            return;
        }
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("OLED ready");
    display.display();
}



// buzzer on
void Actuators::buzzer_on() {
    ledcWriteTone(BUZZER_CHANNEL, BUZZER_FREQUENCY);
}

// buzzer off
void Actuators::buzzer_off() {
    ledcWriteTone(BUZZER_CHANNEL, 0);
}

// buzzer beep tone
void Actuators::buzzer_beep() {
    buzzer_on();
    delay(200);
    buzzer_off();
}



// LED PWM
void Actuators::led_pwm(int brightness) {
    ledcWrite(LED_CHANNEL, brightness);
}

// LED fade
void Actuators::led_fade() {

    // darkening
    for (int i = 0; i < 255; i++) {
        ledcWrite(LED_CHANNEL, i);
        delay(5);
    }

    // brightening
    for (int i = 255; i > 0; i--) {
        ledcWrite(LED_CHANNEL, i);
        delay(5);
    }
}


// LED on
void Actuators::led_on() {
    digitalWrite(LED, HIGH);
}

// LED off
void Actuators::led_off() {
    digitalWrite(LED, LOW);
}



// send data to motor
void Actuators::motor_send(uint8_t id, uint8_t mode, uint8_t dir, uint16_t param) {
    uint8_t package[6];

    package[0] = 0xAA;
    package[1] = id;
    package[2] = mode;
    package[3] = dir;
    package[4] = (param >> 8) & 0xFF;   // param = 16 bit -> has to be cutted in 2 bytes    // MSB
    package[5] = param & 0xFF;                                                              // LSB

    Serial2.write(package, 6);
}

// motor step mode
void Actuators::motor_step(uint8_t id, bool dir, uint16_t steps) {
    motor_send(id, 0, dir, steps);
}

// motor permanent run mode
void Actuators::motor_run(uint8_t id, bool dir, uint16_t speed) {
    motor_send(id, 1, dir, speed);
}

// id == 1 -> left; id == 2 -> motor right
// mode == 0 -> step; mode == 1 -> continuous mode
// dir == 0 -> forward; dir == 1 -> backward
// param == steps or speed



// OLED print text
void Actuators::oled_print(String text) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(text);
    display.display();
}
