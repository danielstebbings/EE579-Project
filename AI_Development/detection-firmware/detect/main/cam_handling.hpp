#include "esp_err.h"
#include "model_variables.h"
#include "classifier/ei_classifier_types.h"

#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM

#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 10
#define SIOD_GPIO_NUM 40
#define SIOC_GPIO_NUM 39

#define Y9_GPIO_NUM 48
#define Y8_GPIO_NUM 11
#define Y7_GPIO_NUM 12
#define Y6_GPIO_NUM 14
#define Y5_GPIO_NUM 16
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 17
#define Y2_GPIO_NUM 15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM 47
#define PCLK_GPIO_NUM 13

#define LED_GPIO_NUM 21


#define EI_CAMERA_RAW_FRAME_BUFFER_COLS 320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 240
#define EI_CAMERA_FRAME_BYTE_SIZE 3

const ei_impulse_t impulse          = impulse_640180_0;
ei_impulse_handle_t impulse_handle  = impulse_handle_640180_0;

esp_err_t camera_init();
esp_err_t camera_capture();

int feature_get_data(size_t offset, size_t length, uint8_t* out_ptr);

EI_IMPULSE_ERROR run_model(const ei_impulse_t* impulse, signal_t* image, ei_impulse_result_t* result);

EI_IMPULSE_ERROR detect(ei_impulse_result_t* result);
