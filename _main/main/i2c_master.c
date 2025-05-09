#include "i2c_master.h"

//initialise master
esp_err_t init_i2c_master(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_DATA,
        .scl_io_num = I2C_CLOCK,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000
    };
    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode,0,0,0);
}

static const char *TAG = "ESP_MASTER";
//read a sequency of bytes
esp_err_t esp32_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, ESP_ADDR, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}


esp_err_t esp32_register_write(uint8_t reg_addr, uint8_t *data, size_t len)
{
    int ret;
    if(data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t write_buf[len + 1];

    //write_buf[0] = reg_addr;
    memcpy(write_buf + 1, data, len);

    ret = i2c_master_write_to_device(I2C_MASTER_NUM, ESP_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Successfully wrote: %d bytes to address: %d", len, reg_addr);
    } else {
        ESP_LOGE(TAG, "Failed to write data: %s ", esp_err_to_name(ret));
    }

    return ret;
}
