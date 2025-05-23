#include "motors.h"

static const char *TAG = "ROBOT_ARM";

void setupMotors()
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 50, // 50Hz for servo and ESC
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    // Motor channel
    ledc_channel_config_t motor_channel = {
        .gpio_num = MOTOR_PWM_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&motor_channel);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, (1500 * (1 << 13)) / 20000);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    
    // Servo channel
    ledc_channel_config_t servo_channel = {
        .gpio_num = SERVO_PWM_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&servo_channel);

    // Steering Servo PWM
ledc_channel_config_t steering_channel = {
    .gpio_num = STEERING_SERVO_GPIO,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .channel = LEDC_CHANNEL_2,
    .timer_sel = LEDC_TIMER_0,
    .duty = 0,
    .hpoint = 0
};
ledc_channel_config(&steering_channel);

}

void set_arm_sequence()
{
    //arm 
    ESP_LOGI("MOTORS", "ARMING MOTORS");
    set_motor_speed(1500);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    set_motor_speed(1500);
    vTaskDelay(500/portTICK_PERIOD_MS);
    ESP_LOGI("MOTORS", "ARMING COMPLETE");
}

void set_motor_speed(uint32_t speed_us)
{
    uint32_t duty = (speed_us * (1 << 13)) / 20000;
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

    ESP_LOGI("MOTORS", "Set motors to %lu duty", duty);
}

void set_motor_reverse()
{
    set_motor_speed(1500);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    set_motor_speed(1100);
    vTaskDelay(500/portTICK_PERIOD_MS);
    set_motor_speed(1500);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    set_motor_speed(1100);
    vTaskDelay(500/portTICK_PERIOD_MS);
    set_motor_speed(1500);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    set_motor_speed(1100);
}

bool get_motors_speed()
{
    uint32_t duty = ledc_get_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

    if(duty != 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

// Set servo angle (0-180 degrees)
void set_servo_angle(int angle)
{
    int min_us = 600; // PWM for the Robot arm servo
    int max_us = 2400;
    int us = min_us + ((max_us - min_us) * angle) / 180;
    uint32_t duty = (us * (1 << 13)) / 20000;
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    
    ESP_LOGI(TAG, "Set servo to %lu duty", duty);
}

// Control magnet
void magnet_on() { gpio_set_level(MAGNET_GPIO, 1); }
void magnet_off() { gpio_set_level(MAGNET_GPIO, 0); }


void set_steering_angle(int angle)
{
    int min_us = 600;   //PWM signal for the Steering servo
    int max_us = 2400; // ''
    int us = min_us + ((max_us - min_us) * angle) / 180;
    uint32_t duty = (us * (1 << 13)) / 20000;
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);

    ESP_LOGI(TAG, "Set steering angle: %d degrees", angle);
}
