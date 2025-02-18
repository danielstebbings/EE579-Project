#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

static const char *TAG = "example";

#define TRIGGER_ULTRASONIC 23
#define ECHO_ULTRASONIC 22

const float SOUND_SPEED = 0.0343; // Speed of sound in cm/us
const uint64_t ECHO_TIMEOUT = 20000; // Timeout for echo pulse (20ms)

static void configure_io(void)
{
    ESP_LOGI(TAG, "Hardware initialised!\n");

    // Configure trigger pin as output
    gpio_reset_pin(TRIGGER_ULTRASONIC);
    gpio_set_direction(TRIGGER_ULTRASONIC, GPIO_MODE_OUTPUT);

    // Configure echo pin as input
    gpio_reset_pin(ECHO_ULTRASONIC);
    gpio_set_direction(ECHO_ULTRASONIC, GPIO_MODE_INPUT);
}

float objectDistance()
{
    uint64_t start_time, end_time;

    // Send a 10us pulse to the trigger pin
    gpio_set_level(TRIGGER_ULTRASONIC, 0);
    esp_rom_delay_us(2);
    gpio_set_level(TRIGGER_ULTRASONIC, 1);
    esp_rom_delay_us(100);
    gpio_set_level(TRIGGER_ULTRASONIC, 0);

    // Wait for the echo pin to go HIGH (start of echo pulse)
    uint64_t timeout = esp_timer_get_time() + ECHO_TIMEOUT;
    ESP_LOGI(TAG, "Waiting for echo start...");
    while (gpio_get_level(ECHO_ULTRASONIC) == 0) {
        ESP_LOGI(TAG, "Echo state: %d", gpio_get_level(ECHO_ULTRASONIC));
        if (esp_timer_get_time() > timeout) {
            ESP_LOGE(TAG, "Echo pulse start timeout");
            return -1.0;
        }
    }
    
    start_time = esp_timer_get_time();  // Correct place to record start time

    // Wait for the echo pin to go LOW (end of echo pulse)
    timeout = esp_timer_get_time() + ECHO_TIMEOUT;
    while (gpio_get_level(ECHO_ULTRASONIC) == 1) {
        if (esp_timer_get_time() > timeout) {
            ESP_LOGE(TAG, "Echo pulse end timeout");
            return -1.0; // Error value
        }
    }
    end_time = esp_timer_get_time();  // Record end time after loop exits

    // Calculate pulse duration
    uint64_t pulse_duration = end_time - start_time;

    // Calculate distance in cm
    float distance = (pulse_duration * SOUND_SPEED) / 2.0;

    ESP_LOGI(TAG, "Pulse duration: %llu us, Distance: %.2f cm", pulse_duration, distance);
    return distance;
}

void app_main(void)
{
    // Configure GPIO pins
    configure_io();

    while (1) {
        // Measure distance
        float distance = objectDistance();
        if (distance > 0) {
            ESP_LOGI(TAG, "Distance: %.2f cm", distance);
        } else {
            ESP_LOGE(TAG, "Failed to measure distance");
        }

        // Delay before next measurement
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}