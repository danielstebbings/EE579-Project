#include "i2c_slave.h"

#define I2C_SLAVE_NUM 0
#define ESP_ADDR 0x58
#define I2C_SLAVE_TIMEOUT_MS 500
#define I2C_RX_BUF_LEN 256
#define I2C_TX_BUF_LEN 256


static const char *TAG = "I2C_SLAVE"; 

esp_err_t init_i2c_slave(void)
{
    int i2c_slave_port = I2C_SLAVE_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_SLAVE,
        .sda_io_num = I2C_DATA,
        .scl_io_num = I2C_CLOCK,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .slave.slave_addr = ESP_ADDR,
        .slave.addr_10bit_en = 0,
    };
    i2c_param_config(i2c_slave_port, &conf);
    
    return i2c_driver_install(i2c_slave_port, conf.mode, I2C_TX_BUF_LEN, I2C_RX_BUF_LEN, 0);
}

//read data from the master
esp_err_t i2c_slave_read(uint8_t *data, size_t len)
{
    uint32_t startMs = esp_timer_get_time() / 1000;
    int bytes_read = i2c_slave_read_buffer(I2C_SLAVE_NUM, data, len, I2C_SLAVE_TIMEOUT_MS / portTICK_PERIOD_MS);
    uint32_t stopMs = esp_timer_get_time() / 1000;

    uint8_t value_data[bytes_read]; //store value
    if (bytes_read > 0)
    {
        ESP_LOGI(TAG, "Read %d bytes from master", bytes_read);

        for(int i = 0; i < bytes_read; i++) {
            //ESP_LOGI(TAG, "I2C_SLAVE_READ_BUFFER: %u", *data);
            if(i % 2 == 0)
            {
                uint8_t value_data = data[i];   //store value
            }
            ESP_LOGI(TAG, "Byte %d: 0x%02x (%c)", i, data[i], isprint(data[i]));
        }
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Nothing recieved len: %d, waited: %lu ms",bytes_read, stopMs - startMs);
        return ESP_FAIL;
    }
}

// Write data to master8
esp_err_t i2c_slave_write(const uint8_t *data, size_t len)
{
    int bytes_written = i2c_slave_write_buffer(I2C_SLAVE_NUM, data, len, I2C_SLAVE_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (bytes_written == len) {
        ESP_LOGI(TAG, "Sent %d bytes to master", bytes_written);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to write full data to master");
        return ESP_FAIL;
    }
}