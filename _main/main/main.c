//#include <Arduino.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ultrasonic.h"

/*
 * TODO: MERGE JAMES CODE
 */

static const char *TAG = "ULTRASONIC";


void app_main(void) {  

  setupUltrasonic(ECHO_PIN_US1, TRIG_PIN_US1, 1);

  while(1)
  {
    float distance = measure_distance(ECHO_PIN_US1);

    ESP_LOGI(TAG, "Distance measured by ultrasonic : %f", distance);
    
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
