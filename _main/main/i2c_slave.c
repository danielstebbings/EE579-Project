#include "i2c_slave.h"

#define SLAVE_REQUEST_WAIT_MS 100

/*
    TODO: SETUP TO RECIEVE AN INTERRUPT FROM SECONARY ESP.
*/

static const char *TAG = "I2C-SLAVE";
i2c_cmd_handle_t slave_handle;

void init_i2c_slave(void) {
    i2c_config_t i2c_slv_config = {
        .sda_io_num = 21,  // SDA pin (set to GPIO 21)
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = 22,  // SCL pin (set to GPIO 22)
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .mode = I2C_MODE_SLAVE,
        .slave.addr_10bit_en = 0,
        .slave.slave_addr = 0x58,  // Slave address 0x58
    };

    // Initialize the I2C slave interface
    esp_err_t err = i2c_param_config(I2C_NUM_0, &i2c_slv_config);
    if (err != ESP_OK) {
        ESP_LOGE("I2C_SLAVE", "I2C slave configuration failed");
        return;
    }

    // Install the I2C driver for slave mode with RX and TX buffer lengths set to 256 bytes each
    err = i2c_driver_install(I2C_NUM_0, I2C_MODE_SLAVE, 256, 256, 0);
    if (err != ESP_OK) {
        ESP_LOGE("I2C_SLAVE", "I2C slave driver installation failed");
        return;
    }

    ESP_LOGI("I2C_SLAVE", "Slave initialized on port %d, address 0x%02X", I2C_NUM_0, 0x58);
}


uint8_t inBuff[256];
uint16_t inBuffLen = 0;
bool check_for_data() {
    uint32_t startMs = esp_timer_get_time() / 1000;
    size_t size = i2c_slave_read_buffer(I2C_NUM_0, inBuff, 1, 1000 / pdMS_TO_TICKS(1000));
    uint32_t stopMs = esp_timer_get_time() / 1000;
  
    if (size > 0) { // If any data is received
      inBuffLen = size;
      ESP_LOGI(TAG, "Received %d bytes:", inBuffLen);
      ESP_LOG_BUFFER_HEX(TAG, inBuff, inBuffLen);
      return true;
    }
  
    ESP_LOGI(TAG, "No data received, waited: %lu ms", stopMs - startMs);
    return false;
  }
  
  /**
   * I2C slave handling task
   */
void i2c_slave_task(void *arg) {
    while (1) {
      if (check_for_data()) {
        // Data received and logged
        // You can process the received data here if needed
      }
      vTaskDelay(pdMS_TO_TICKS(SLAVE_REQUEST_WAIT_MS));
    }
    vTaskDelete(NULL);
}