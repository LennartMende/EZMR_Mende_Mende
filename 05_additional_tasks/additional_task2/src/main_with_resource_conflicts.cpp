#include <Arduino.h>


static const BaseType_t app_cpu = 0;


const char msg[] = "This is a random string which wants to be interrupted!";

static TaskHandle_t task_1 = NULL;
static TaskHandle_t task_2 = NULL;

void startTask1(void *parameter) {
  int msg_len = strlen(msg);
  while (1) {
    Serial.println();
    for (int char_idx = 0; char_idx < msg_len; char_idx++) {
      Serial.print(msg[char_idx]);
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    Serial.println();
  }
}

void startTask2(void *parameter) {
  while (1) {
    Serial.println("XXXXXXXXXX");
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup() {
  // low baud rate to watch the process
  Serial.begin(300);

  vTaskDelay(1000/ portTICK_PERIOD_MS); // to wait for the Serial to connect
  Serial.println();
  
  Serial.print("Setup and loop task running on core ");
  Serial.print(xPortGetCoreID());
  Serial.print(" with priority ");
  Serial.println(uxTaskPriorityGet(NULL));

  xTaskCreatePinnedToCore(startTask1,
    "Task 1",
    1024,
    NULL,
    1,
    &task_1,
    app_cpu
  );
  xTaskCreatePinnedToCore(startTask2,
    "Task 2",
    1024,
    NULL,
    2,
    &task_2,
    app_cpu
  );
}


void loop() {
  vTaskDelay(portMAX_DELAY); // to avoid using CPU time again and again
}