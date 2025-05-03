#ifndef __I2C_SLAVE_H__
#define __I2C_SLAVE_H__

/*
    FOR MAIN ESP - RECIEVE DATA FROM DANIELS CAMERA
*/

//#include "driver/i2c.h"       //legacy driver
//#include "driver/i2c_slave.h"
#include "string.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define I2C_CLOCK 22
#define I2C_DATA 21

void init_i2c_slave(void);
bool check_for_data(void);
void i2c_slave_task(void *arg);

#endif //__I2C_SLAVE_H__