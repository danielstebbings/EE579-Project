/*
*   main.c
*   ESP32_MASTER PINOUT
*   
*/
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ultrasonic.h"
#include "motors.h"
#include "bluetooth.h"
#include "i2c_master.h"
#include "i2c_slave.h"

static const char* TAG_BT = "BLUETOOTH";
// Bluetooth Event Handler
void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event)
    {
    case ESP_SPP_INIT_EVT:
        esp_bt_dev_set_device_name(DEVICE_NAME);
        esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME);
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        break;
  
    case ESP_SPP_START_EVT:
        ESP_LOGI(TAG_BT, "SPP Server Started");
        break;
    case ESP_SPP_DATA_IND_EVT:
        param->data_ind.data[param->data_ind.len] = 0; // Null terminate incoming data
        ESP_LOGI(TAG_BT, "Received: %s", (char *)param->data_ind.data);

        if (strcmp((char *)param->data_ind.data, "on") == 0)
        {
            magnet_on();
        }
        else if (strcmp((char *)param->data_ind.data, "off") == 0)
        {
            magnet_off();
        }
        else if (strstr((char *)param->data_ind.data, "angle:") != NULL)
        {
            int angle = atoi((char *)param->data_ind.data + 6);
            set_servo_angle(angle);
            ESP_LOGI(TAG_BT, "Set servo angle to: %d degrees", angle);
        }
        break;
    default:
        break;
    }
}


void bluetooth_init()
{
    esp_err_t ret = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    if (ret) { ESP_LOGW(TAG_BT, "Bluetooth controller release BLE memory failed: %s", esp_err_to_name(ret)); }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) { ESP_LOGE(TAG_BT, "Bluetooth controller initialize failed: %s", esp_err_to_name(ret)); }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret) { ESP_LOGE(TAG_BT, "Bluetooth controller enable failed: %s", esp_err_to_name(ret)); }

    ret = esp_bluedroid_init();
    if (ret) { ESP_LOGE(TAG_BT, "Bluedroid initialize failed: %s", esp_err_to_name(ret)); }

    ret = esp_bluedroid_enable();
    if (ret) { ESP_LOGE(TAG_BT, "Bluedroid enable failed: %s", esp_err_to_name(ret)); }

    esp_spp_register_callback(esp_spp_cb);
    esp_spp_init(ESP_SPP_MODE_CB);
}


//TODO: Setup freeRTOS tasks.



static const char *TAG_US = "ULTRASONIC";

void app_main(void) { 
  
  bluetooth_init();
  setupMotors();
  setupUltrasonic(ECHO_PIN_US1, TRIG_PIN_US1, 1); //front
  //setupUltrasonic(ECHO_PIN_US2, TRIG_PIN_US2, 2);
  //setupUltrasonic(ECHO_PIN_US3, TRIG_PIN_US3, 3);

  esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //i2c error check
    init_i2c_slave();

    //init_i2c_master();    //--disabled on slave

    //set default
    set_motor_speed(1500);
    set_servo_angle(90);
    set_steering_angle(90);


    uint8_t recv_date[256] = {0};
  while(1)
  {
    float distance_front = measure_distance(ECHO_PIN_US1);
    float distance_left;
    float distance_right;

    //ESP_LOGI(TAG_US, "Distance measured by ultrasonic : %f", distance_front);
    ESP_LOGI("TEST", "LOOP SUCCESS");
    
    //Recieve test data
    i2c_slave_read(recv_date, sizeof(recv_date));

    //Write test data
    //esp32_register_write(ESP_ADDR, 0b10101010);
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
