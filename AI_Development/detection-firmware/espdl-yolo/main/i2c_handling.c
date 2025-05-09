#include "driver/i2c_master.h"
#include  "i2c_handling.h"
static const char *TAG = "i2c_handling";
esp_err_t register_write_bytes(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data[5])
{
    uint8_t write_buf[6] = {reg_addr, data[0],data[1],data[2],data[3],data[4],};
    ESP_LOGI(TAG, "BUF: A %02X | H %02X | C0 %02X | C1 %02X | C2 %02X | C3 %02X |", write_buf[0],write_buf[1],write_buf[2],write_buf[3],write_buf[4],write_buf[5]);
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief i2c master initialization
 */
void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_XTAL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length    = I2C_ADDR_BIT_LEN_7,
        .device_address     = ESP_ADDR,
        .scl_speed_hz       = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));
}