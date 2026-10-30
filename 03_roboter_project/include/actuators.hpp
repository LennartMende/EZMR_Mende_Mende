#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>



class Actuators{
public:

    // setup
    void setup_buzzer();
    void setup_led_on_off();
    void setup_led_pwm();
    void setup_motor();
    void setup_oled();

    // buzzer
    void buzzer_on();
    void buzzer_off();
    void buzzer_beep();

    // PWM - empty
    void led_pwm(int brightness);
    void led_fade();

    // LED
    void led_on();
    void led_off();

    // motor
    void motor_send(uint8_t id, uint8_t mode, uint8_t dir, uint16_t param);
    void motor_step(uint8_t id, bool dir, uint16_t steps);
    void motor_run(uint8_t id, bool dir, uint16_t speed);

    // OLED
    void oled_init();
    void oled_print(String text);

//private:

    const int BUZZER = 4;
    const int BUZZER_CHANNEL = 0;
    const int BUZZER_FREQUENCY = 350;

    const int PWM[3] = {5, 18, 19};

    static constexpr int LED_CHANNEL = 1;
    static constexpr int LED_FREQ = 5000;
    static constexpr int LED_RESOLUTION = 8;

    const int LED = 15;

    const int MOTOR = 17;

    const int OLED_SDA = 21;
    const int OLED_SCL = 22;

    static constexpr int SCREEN_WIDTH = 128;
    static constexpr int SCREEN_HEIGHT = 64;

    Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

    //const int INFRARED_SENDER = 32;
    //const int INFRARED_RECEIVER = 33;
};
