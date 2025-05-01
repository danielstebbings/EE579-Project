
#include "cam_handling.hpp"

#include "classifier/ei_classifier_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void) {
    ei_impulse_result_t* result = new ei_impulse_result_t();
    
    camera_init();

    while(1) {
        detect(result);
        
        vTaskDelay(5000 / portTICK_RATE_MS);
    }
    
    return;

}