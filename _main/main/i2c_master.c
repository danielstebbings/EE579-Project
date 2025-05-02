#include "i2c_master.h"

i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t dev_handle;

void init_i2c_master(void)
{
    i2c_master_bus_config_t i2c_master_cfg = { //TODO: define the vars with 0.
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_CLOCK,    //clock
        .sda_io_num = I2C_DATA,    //data
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master_cfg, &bus_handle));


    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x58,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

}

i2c_master_bus_handle_t get_bus_handle(void)
{
    return bus_handle;
}
i2c_master_dev_handle_t get_dev_handle(void)
{
    return dev_handle;
}