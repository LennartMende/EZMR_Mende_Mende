#include <Arduino.h>


// blinky every second on core 0
BaseType_t led_core = 0;
static const int LED_PIN = 15;
//Our task: Blink an LED
void toggleLED(void* parameter) {
  while(1) {
    digitalWrite(LED_PIN, HIGH);
    // now use of vTaskDelay() as non-blocking delay function, so othere tasks can run in the meantime
    vTaskDelay(1000 / portTICK_PERIOD_MS);// vTaskDelay(ticks) with ticks==1000 ->vTaskDelay(1000)
                                          // portTICK_PERIOD_MS == 1 per default, vTaskDelay needs tick count 
                                          // for scheduler to determine the next task after portTICK_PERIOD_MS ms
    Serial.printf("Core of LED task: %d\n", xPortGetCoreID());
    digitalWrite(LED_PIN, LOW);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.printf("Core of LED task: %d\n", xPortGetCoreID());
  }
}


// buzzer one sec on and one sec off
const int BUZZER = 4;
const int BUZZER_CHANNEL = 0;
const int BUZZER_FREQUENCY = 350;
void setup_buzzer() {
    ledcAttach(BUZZER, 350, 8);
}
void buzzer_on() {
    ledcWriteTone(BUZZER, 350);
}
void buzzer_off() {
    ledcWriteTone(BUZZER, 0);
}
BaseType_t buzzer_core = 1;
void toggleBuzzer(void* parameter) {
  while(1) {
    buzzer_on();
    // now use of vTaskDelay() as non-blocking delay function, so othere tasks can run in the meantime
    vTaskDelay(1000 / portTICK_PERIOD_MS);// vTaskDelay(ticks) with ticks==1000 ->vTaskDelay(1000)
                                          // portTICK_PERIOD_MS == 1 per default, vTaskDelay needs tick count 
                                          // for scheduler to determine the next task after portTICK_PERIOD_MS ms
    Serial.printf("Core of buzzer task: %d\n", xPortGetCoreID());
    buzzer_off();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.printf("Core of buzzer task: %d\n", xPortGetCoreID());
  }
}



void setup() {
  pinMode(LED_PIN, OUTPUT);
  xTaskCreatePinnedToCore(
    toggleLED, // function to be called
    "Toggle LED", // name of task
    1024, // stack size (bytes in ESP32, words in FreeRTOS)
    NULL, // params to pass to function
    1, // task priority (0 to configMAX_PRORITIES-1)
    NULL, // task handle
    led_core // run on core 0
  );
  setup_buzzer();
  xTaskCreatePinnedToCore(
    toggleBuzzer, // function to be called
    "Toggle buzzer", // name of task
    2024, // stack size (bytes in ESP32, words in FreeRTOS), needs more mem because more complex than blinky
    NULL, // params to pass to function
    1, // task priority (0 to configMAX_PRORITIES-1)
    NULL, // task handle
    buzzer_core // run on core 1
  );
  Serial.begin(115200);
}


void loop() {
  
}
