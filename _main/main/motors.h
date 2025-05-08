#ifndef __MOTORS_H__
#define __MOTORS_H__

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
#define STEERING_SERVO_GPIO 22

void setupMotors();
void set_motor_speed(uint32_t speed_us);
bool get_motor_speed();
void set_motor_reverse(uint32_t speed_us);
void set_servo_angle(int angle);
void magnet_on();
void magnet_off();
void set_steering_angle(int angle);

#endif //__MOTORS_H__
