
#include "cam_handling.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {
    ei_impulse_result_t* result;
    
    camera_init();

    while(1) {
        detect(ei_impulse_result_t* result);
        
        vTaskDelay(5000 / portTICK_RATE_MS);
    }
    


}