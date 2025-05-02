#ifndef __ULTRASONIC_H__
#define __ULTRASONIC_H__

/*
    EE579 MCU April 2025
    ultrasonic.h

    PWM timer used for the ultrasonic sensor, 10us high time and variable low time
    dependant on the distance being measured.
*/

#include "esp_timer.h"
#include "driver/ledc.h"
#include "driver/gpio.h"


#define SOUND_SPEED 0.034

#define TRIG_PIN_US1 25
#define ECHO_PIN_US1 26
#define TRIG_PIN_US2 99     //TODO: DEFINE PINS
#define ECHO_PIN_US2 99
#define TRIG_PIN_US3 99
#define ECHO_PIN_US3 99

extern volatile bool timer_state;
extern volatile int distance;

//define onboard LED for debugging.
#define LED 2

void setupUltrasonic(int ECHO_PIN, int TRIG_PIN, int CHANNEL);
float measure_distance(int ECHO_PIN);

#endif //__ULTRASONIC_H__