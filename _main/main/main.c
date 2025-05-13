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

#define DISTANCE_THRESHOLD 30
#define MOTOR_05_M 500             //in ms TODO: TEST AND MODIFY
#define FWD_FAST 1650
#define FWD_MID 1600
#define FWD_SLOW 1570
#define MOTOR_NUETRAL 1500
#define RVRS_SLOW 1400
#define RVRS_FAST 1200
#define IDENTIFY_TIMEOUT 1337
#define SEARCH_TIMEOUT 2000


static const char* TAG_BT = "BLUETOOTH";
// Bluetooth Event Handler
bool start_sequence = false;
bool magnet_enabled = false;
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
            magnet_enabled = true;
            
        }
        else if (strcmp((char *)param->data_ind.data, "off") == 0)
        {
            magnet_off();
            magnet_enabled = false;
        }
        else if (strcmp((char *)param->data_ind.data, "start") == 0)
        {
            //start the drive sequence
            start_sequence = false;

        }
        else if (strcmp((char *)param->data_ind.data, "stop") == 1)
        {
            //stop the drive sequence
            start_sequence = true;
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


static const char *TAG_US = "ULTRASONIC";

void app_main(void) { 
 
    ESP_LOGI("APP_MAIN", "PLS WORK 1");
    bluetooth_init();
    setupMotors();
    set_arm_sequence(); 

    setupUltrasonic(ECHO_PIN_US1, TRIG_PIN_US1, 1); //front
    setupUltrasonic(ECHO_PIN_US2, TRIG_PIN_US2, 2);
    setupUltrasonic(ECHO_PIN_US3, TRIG_PIN_US3, 3);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //i2c error check
    init_i2c_slave();

    //set default   --TODO: DOUBLE CHECK ON TEST
    set_motor_speed(0);
    set_servo_angle(0);
    set_steering_angle(90);

    enum State {
        START,
        APPROACH,
        APPROACH_LOOK,
        APPROACH_IDENTIFIED,
        SEARCH_LOOK,
        SEARCH_MOVE,
        LOST,
        LOST_MOVE,
        LOST_SEARCH,
        COLLECT_MOVE,
        COLLECT_WAIT,
        FINISH_MOVE,
        FINISH_WAIT
    };
    enum State current_state = START;
    int n_inferences = 0;

    int i = 0;
    float distance_front[4];
    float distance_front_avg;
    int search_count = 0;   //keep track how many times weve been here

    while(1) {

    if (i > 4)
    {
        i = 0;
    }
    
    distance_front[i] = measure_distance(ECHO_PIN_US1);
    float distance_left = measure_distance(ECHO_PIN_US2);
    float distance_right = measure_distance(ECHO_PIN_US3);

    /*
    CONTROL LOGIC

    scan left and right -- try ignore which side the opposition car is at
    -- if big jump can ignore - end of lockers
    -- if we are stationary and the distance changes on a side - then that is the car.

    camera for directional front servo control
    -- send left half and send right half (0 - 255)

    combine this data with ultrasonic data to determine how large of an angle is required
    --smaller angle at longer distance 

    01 - where are we
    02 - wait for begin -- bluetooth
    03 - approach the can -- drive forwards 0.5
    04 - stop at X distance and look at front ultrasonic and camera
    05 - if no object go back to 4
    06 - else approach the can
    */

    //Recieve camera data
    uint8_t recv_date[6] = {0};
    i2c_slave_read(recv_date, sizeof(recv_date));
    ESP_LOGI(TAG_BT, "BUF: A %02X | H %02X | C0 %02X | C1 %02X | C2 %02X | C3 %02X |", recv_date[0],recv_date[1],recv_date[2],recv_date[3],recv_date[4],recv_date[5]);
    uint8_t camera_header = recv_date[1];
    uint8_t camera_angle_1 = recv_date[2];
    uint8_t camera_angle_2 = recv_date[3];
    uint8_t camera_angle_3 = recv_date[4];
    uint8_t camera_angle_4 = recv_date[5];

    //split camera header
    /*
    bit 0-3 - how many objects detected 
    bit 4-7 - 0 for left, 1 for right - the bit that it is in will be what objects its for
    */
   ESP_LOGI("APP_MAIN", "PLS WORK 3");

    uint8_t n = camera_header && 0x0F;
    uint8_t l1 = (camera_header && BIT4) >> 4;
    uint8_t l2 = (camera_header && BIT5) >> 4;
    uint8_t l3 = (camera_header && BIT6) >> 4;
    uint8_t l4 = (camera_header && BIT7) >> 4;
    
    
    /* 
        from Daniels camera - uint8_t
        byte[0] - address
        byte[1] - [0-2 how many objects][]
        byte[2] - 0-255 of can centre point - where 0 is the centre point 255 is far to the side
    */

    static const char *TAG_R = "Running";
    switch (current_state)
    {
    case START:
        ESP_LOGI(TAG_R, "STATE: START");
        if(start_sequence || true) {
            current_state = APPROACH;
        }
        break;

    case APPROACH:
        ESP_LOGI(TAG_R, "STATE: APPROACH");
        //drive ~0.5m then stop
        uint32_t start_time = esp_timer_get_time();
        set_motor_speed(FWD_FAST);
        if (((esp_timer_get_time() - start_time) / 1000) > MOTOR_05_M) {
            set_motor_speed(MOTOR_NUETRAL);
        }
        set_motor_speed(MOTOR_NUETRAL);
        
        ESP_LOGI(TAG_R, "STATE: APPROACH COMPLETE");
        current_state = APPROACH;
        break;  

    case APPROACH_LOOK:
        ESP_LOGI(TAG_R, "STATE: APPROACH_LOOK");
        //detect what object we want to track onto
        n_inferences++;     //count how many inferences we look at
        start_time = esp_timer_get_time();  //count how much time

        for(int j = 0; j <= sizeof(distance_front); j++)
        {
            distance_front_avg = distance_front[j] / 4;

            //if avg distance buffer < threshold or camera detect go to approach
            if(distance_front_avg <= DISTANCE_THRESHOLD || n > 0)   //TODO: will need tidy up on n>0
            {
                //SOMETHING THERE
                current_state = APPROACH_IDENTIFIED;
            }
        }
        //if been in state for too long - time or inference -- go to search look
        if(((esp_timer_get_time() - start_time) / 1000) > IDENTIFY_TIMEOUT)
        {
            current_state = SEARCH_LOOK;
        }
        
        break;

    case APPROACH_IDENTIFIED:
        ESP_LOGI(TAG_R, "STATE: APPROACH_IDENTIFIED");
        //drive towards can
        set_motor_speed(FWD_SLOW);

        //once under threshold distance - move to collect move
        if(distance_front[0] < DISTANCE_THRESHOLD)  //TODO: fix
        {
            set_motor_speed(MOTOR_NUETRAL);
            current_state = COLLECT_MOVE;
        }
        
        break;

    case SEARCH_LOOK:
        ESP_LOGI(TAG_R, "STATE: SEARCH_LOOK");
        //like approach look except we keep track how long we are here
        search_count++;
        start_time = esp_timer_get_time();

        //if we been in too many times - go to lost move
        if(search_count > 5 || ((esp_timer_get_time() - start_time) / 1000) > SEARCH_TIMEOUT)
        {
            current_state = LOST_MOVE;
        }
        else
        {
            for(int j = 0; j <= sizeof(distance_front); j++)
            {
                distance_front_avg = distance_front[j] / 4;
                //if avg distance buffer < threshold or camera detect go to approach
                if(distance_front_avg <= DISTANCE_THRESHOLD || n > 0)   //TODO: will need tidy up on n>0
                {
                    //SOMETHING THERE
                    current_state = APPROACH_IDENTIFIED;
                }
                else
                {
                    //goes to search move if does not find
                    current_state = SEARCH_MOVE;
                }
            }
        }
        break;

    case SEARCH_MOVE:
        ESP_LOGI(TAG_R, "STATE: SEARCH_MOVE");
        //drive forward x amount ~ 1m
        start_time = esp_timer_get_time();
        set_motor_speed(FWD_SLOW);
        if (((esp_timer_get_time() - start_time) / 1000) > 2*MOTOR_05_M) {
            set_motor_speed(MOTOR_NUETRAL);
        }

        //hit a threshold for ultrasonic or stop after predetermined time
        //go back to search look
        break;

    case LOST_MOVE:
        ESP_LOGI(TAG_R, "STATE: LOST_MOVE");
        //Drive backwards
        start_time = esp_timer_get_time();
        set_motor_reverse(MOTOR_NUETRAL);
        
        if (((esp_timer_get_time() - start_time) / 1000) > 4*MOTOR_05_M) {
            set_motor_speed(MOTOR_NUETRAL);
        }
        //if detect go 
        if(distance_front_avg <= DISTANCE_THRESHOLD || n >= 1)
        {
            current_state = APPROACH_IDENTIFIED;
        }
        else
        {
            current_state = LOST_SEARCH;
        }
        //else lost search
        break;

    case LOST_SEARCH:
        ESP_LOGI(TAG_R, "STATE: LOST_SEARCH");
        //like search look except we go backwards
        break;
    
    case COLLECT_MOVE:
        ESP_LOGI(TAG_R, "STATE: COLLECT_MOVE");
        //position magnet over the can
        set_motor_speed(MOTOR_NUETRAL);
        //detect magnet on
        if(magnet_enabled)
        {
        start_time = esp_timer_get_time();
            if (((esp_timer_get_time() - start_time) / 1000) > 3000) {
                current_state = FINISH_MOVE;
            }  
        }
        //once done
        break;

    case COLLECT_WAIT:
        ESP_LOGI(TAG_R, "STATE: COLLECT_WAIT");
        //wait until bluetooth signal cmd done - then finish move
        break;

    case FINISH_MOVE:
        ESP_LOGI(TAG_R, "STATE: FINISH_MOVE");
        //floor it backwards
        start_time = esp_timer_get_time();
        set_motor_reverse();
        if (((esp_timer_get_time() - start_time) / 1000) > 8*MOTOR_05_M) {     //~8m reverse
            set_motor_speed(MOTOR_NUETRAL);
            current_state = FINISH_WAIT;
        }
        
        break;

    case FINISH_WAIT:
        ESP_LOGI(TAG_R, "STATE: FINISH_WAIT");
        //park it
        set_motor_speed(MOTOR_NUETRAL);
        break;

    default:
        ESP_LOGI(TAG_R, "STATE: DEFAULT - GOING TO START");
        current_state = START;
        break;
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); //TODO: ALTER VALUE.
    }
}

