#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

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

#define SPP_SERVER_NAME "ESP32_CAR_ARM"
#define DEVICE_NAME "ESP32_CAR_ARM"

void bluetooth_init();

#endif //__BLUETOOTH_H__