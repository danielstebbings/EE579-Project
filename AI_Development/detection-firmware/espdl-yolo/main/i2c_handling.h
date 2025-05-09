#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_MASTER_NUM 0
#define ESP_ADDR 0x58
#define I2C_MASTER_TIMEOUT_MS 1000

/*
*   FOR DANIELS SECONDARY ESP - SETUP AS MASTER
*/

#include "driver/i2c.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "esp_check.h"
#include <string.h>

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SCL_IO   5
#define I2C_MASTER_SDA_IO    4

esp_err_t register_write_bytes(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data[5]);
void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle);


#ifdef __cplusplus
}
#endif

#endif //__I2C_MASTER_H__