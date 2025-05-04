#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

/*
*   FOR DANIELS SECONDARY ESP - SETUP AS MASTER
*/

//#include "driver/i2c.h"       //legacy driver
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_check.h"

#define I2C_CLOCK 22
#define I2C_DATA 21

void init_i2c_master(void);
i2c_master_bus_handle_t get_bus_handle(void);
i2c_master_dev_handle_t get_dev_handle(void);
void i2c_master_test(void);

#endif //__I2C_MASTER_H__