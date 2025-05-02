#include "esp_log.h"
#include "ultrasonic.h"

void setupUltrasonic(int ECHO_PIN, int TRIG_PIN, int CHANNEL)
{
    //Setup echo pins for ultrasonics 1-3
    gpio_config_t ultrasonic_IO = {
        .pin_bit_mask = (1ULL << ECHO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&ultrasonic_IO);

    //configure timer.
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_17_BIT,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 40,  //TODO: placeholder rn
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);
    //25000us timer


    //trig channel      -- can be replicated just need to change the led channel.
    ledc_channel_config_t trig_channel = {
        .gpio_num = TRIG_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_3,
        .timer_sel = LEDC_TIMER_1,
        .duty = 6,    
        .hpoint = 0 
    };
    ledc_channel_config(&trig_channel);
}


//Measure distance for each ultrasonic
float measure_distance(int ECHO_PIN)
{
    //gpio_set_level(ECHO_PIN, 0);

    while(gpio_get_level(ECHO_PIN) == 0);
    //start timer
    uint32_t start_time = esp_timer_get_time();

    while(gpio_get_level(ECHO_PIN) == 1);
    uint32_t end_time = esp_timer_get_time();
    //gpio_reset_pin(ECHO_PIN);

    uint32_t duration_us = end_time - start_time;
    float distance_cm = (duration_us * SOUND_SPEED) / 2;

    //logging
    //static const char* TAG = "TIMERS";
    //ESP_LOGI(TAG, "TIMER START : %lu , TIMER END : %lu , TIMER DUR : %lu",start_time, end_time, duration_us);

    return distance_cm;
}