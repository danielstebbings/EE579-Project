#ifndef __I2C_SLAVE_H__
#define __I2C_SLAVE_H__

/*
    FOR MAIN ESP - RECIEVE DATA FROM DANIELS CAMERA
*/

//#include "driver/i2c_slave.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_timer.h"
#include <ctype.h>

#define I2C_CLOCK 22
#define I2C_DATA 21

esp_err_t init_i2c_slave(void);
esp_err_t i2c_slave_read(uint8_t *data, size_t len);
esp_err_t i2c_slave_write(const uint8_t *data, size_t len);

#endif //__I2C_SLAVE_H__