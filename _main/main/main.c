//#include <Arduino.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ultrasonic.h"

/*
  TODO:
  1.  Setup timers/PWM signal
  2.  From PWM signal, use the HIGH to trigger the ultrasonic.
  3.  on the LOW, use that time to wait for response.
  4.  As distance becomes smaller - low time becomes smaller - increase frequency to get faster updates
*/

static const char *TAG = "ULTRASONIC";


unsigned int i = 0;
void app_main(void) {  
  setupUltrasonic();

  while(1)
  {
    float distance = measure_distance();

    ESP_LOGI(TAG, "Distance measured by ultrasonic : %f", distance);
    
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
