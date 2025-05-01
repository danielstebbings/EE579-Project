#ifndef __TIMERS_H__
#define __TIMERS_H__

/*
    EE579 MCU April 2025
    Logan Noonan
    timers.h

    PWM timer used for the ultrasonic sensor, 10us high time and variable low time
    dependant on the distance being measured.
*/

//#include <Arduino.h>
#include "esp_timer.h"
#include "driver/ledc.h"
#include "driver/gpio.h"


#define SOUND_SPEED 0.034

#define TRIG_PIN 25
#define ECHO_PIN 26

extern volatile bool timer_state;
extern volatile int distance;

//define onboard LED for debugging.
#define LED 2

void setupUltrasonic();
float measure_distance();

#endif //__TIMERS_H__