#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include "esp_timer.h"
#include "nvs_flash.h" 
#include "esp_gap_bt_api.h"


// GPIO Pin Definitions
#define MOTOR_PWM_GPIO      18
#define SERVO_PWM_GPIO      19
#define MAGNET_GPIO         21
#define TRIG_GPIO           25
#define ECHO_GPIO           26
#define STEERING_SERVO_GPIO 22

#define SPP_SERVER_NAME "ESP32_CAR_ARM"
#define DEVICE_NAME "ESP32_CAR_ARM"

static const char *TAG = "ROBOT_ARM";

static int spp_handle = 0;


// Initialize PWM for motor and servo
void pwm_init()
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 50, // 50Hz for servo and ESC
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    // Motor channel
    ledc_channel_config_t motor_channel = {
        .gpio_num = MOTOR_PWM_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&motor_channel);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, (1500 * (1 << 13)) / 20000);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    
    // Servo channel
    ledc_channel_config_t servo_channel = {
        .gpio_num = SERVO_PWM_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&servo_channel);

    // Steering Servo PWM
ledc_channel_config_t steering_channel = {
    .gpio_num = STEERING_SERVO_GPIO,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .channel = LEDC_CHANNEL_2,
    .timer_sel = LEDC_TIMER_0,
    .duty = 0,
    .hpoint = 0
};
ledc_channel_config(&steering_channel);

}

// Set motor ESC speed
void set_motor_speed(uint32_t speed_us)
{
    uint32_t duty = (speed_us * (1 << 13)) / 20000;
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

// Set servo angle (0-180 degrees)
void set_servo_angle(int angle)
{
    int min_us = 600; // PWM for the Robot arm servo
    int max_us = 2400;
    int us = min_us + ((max_us - min_us) * angle) / 180;
    uint32_t duty = (us * (1 << 13)) / 20000;
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
}

// Control magnet
void magnet_on() { gpio_set_level(MAGNET_GPIO, 1); }
void magnet_off() { gpio_set_level(MAGNET_GPIO, 0); }

// Measure distance using ultrasonic sensor
float measure_distance_cm()
{
    gpio_set_level(TRIG_GPIO, 0);
    esp_rom_delay_us(2);
    gpio_set_level(TRIG_GPIO, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_GPIO, 0);

    while (gpio_get_level(ECHO_GPIO) == 0) {}
    int64_t start = esp_timer_get_time();

    while (gpio_get_level(ECHO_GPIO) == 1) {}
    int64_t end = esp_timer_get_time();

    float duration_us = end - start;
    float distance_cm = duration_us / 58.0;
    return distance_cm;
}

void set_steering_angle(int angle)
{
    int min_us = 600;   //PWM signal for the Steering servo
    int max_us = 2400; // ''
    int us = min_us + ((max_us - min_us) * angle) / 180;
    uint32_t duty = (us * (1 << 13)) / 20000;
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);

    ESP_LOGI(TAG, "Set steering angle: %d degrees", angle);
}

// Bluetooth Event Handler
void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event)
    {
    case ESP_SPP_INIT_EVT:
        esp_bt_dev_set_device_name(DEVICE_NAME);
        esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME);
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        break;
  
    case ESP_SPP_START_EVT:
        ESP_LOGI(TAG, "SPP Server Started");
        break;
    case ESP_SPP_DATA_IND_EVT:
        param->data_ind.data[param->data_ind.len] = 0; // Null terminate incoming data
        ESP_LOGI(TAG, "Received: %s", (char *)param->data_ind.data);

        if (strcmp((char *)param->data_ind.data, "on") == 0)
        {
            magnet_on();
        }
        else if (strcmp((char *)param->data_ind.data, "off") == 0)
        {
            magnet_off();
        }
        else if (strstr((char *)param->data_ind.data, "angle:") != NULL)
        {
            int angle = atoi((char *)param->data_ind.data + 6);
            set_servo_angle(angle);
            ESP_LOGI(TAG, "Set servo angle to: %d degrees", angle);
        }
        break;
    default:
        break;
    }
}

// Initialize Bluetooth SPP server
void bluetooth_init()
{
    esp_err_t ret = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    if (ret) { ESP_LOGW(TAG, "Bluetooth controller release BLE memory failed: %s", esp_err_to_name(ret)); }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) { ESP_LOGE(TAG, "Bluetooth controller initialize failed: %s", esp_err_to_name(ret)); }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret) { ESP_LOGE(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret)); }

    ret = esp_bluedroid_init();
    if (ret) { ESP_LOGE(TAG, "Bluedroid initialize failed: %s", esp_err_to_name(ret)); }

    ret = esp_bluedroid_enable();
    if (ret) { ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret)); }

    esp_spp_register_callback(esp_spp_cb);
    esp_spp_init(ESP_SPP_MODE_CB);
}


void app_main()
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    pwm_init();

    gpio_set_direction(MAGNET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(TRIG_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_GPIO, GPIO_MODE_INPUT);

    bluetooth_init();

    set_motor_speed(1500); // Neutral ESC
    set_servo_angle(90);   // Center servo
    set_steering_angle(90); // Center steering servo
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    while (1)
    {
        float distance = measure_distance_cm();
        printf("Distance: %.2f cm\n", distance);

        if (distance < 60.0)  // Obstacle detected <60cm
  {
   
    set_motor_speed(1500); // Neutral
vTaskDelay(500 / portTICK_PERIOD_MS); // 300ms stop

set_steering_angle(135);  // Steer while reversing

set_motor_speed(1600);   // Turn
vTaskDelay(1000 / portTICK_PERIOD_MS); // 1.5s reverse time

set_steering_angle(90);  // turn right


}
else
{
    // No obstacle — keep driving forward straight
    set_motor_speed(1550);  // Forward drive
    set_steering_angle(90); // Wheels straight
}

vTaskDelay(100 / portTICK_PERIOD_MS);  
    }
}
