#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

#define I2C_MASTER_NUM 0
#define ESP_ADDR 0x58
#define I2C_MASTER_TIMEOUT_MS 1000

/*
*   FOR DANIELS SECONDARY ESP - SETUP AS MASTER
*/

#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_check.h"
#include <string.h>

#define I2C_CLOCK 22
#define I2C_DATA 21

esp_err_t init_i2c_master(void);
esp_err_t esp32_register_read(uint8_t reg_addr, uint8_t *data, size_t len);
esp_err_t esp32_register_write(uint8_t reg_addr, uint8_t *data, size_t len);

#endif //__I2C_MASTER_H__