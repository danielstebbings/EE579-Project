#include "espdet_detect.hpp"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

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

// Image dimensions
#define CAM_WIDTH       640
#define CAM_HEIGHT      480 
#define CAM_PIX_BYTES     3 // 3 Bytes / pixel, RGB888

#define MODEL_WIDTH       416
#define MODEL_HEIGHT      416 

// Classification values
#define CATEGORY_CAN 0
#define CATEGORY_CAR 1

// Results struct


esp_err_t camera_init();
esp_err_t camera_capture(dl::image::img_t &img);

esp_err_t decode_harcoded_jpeg(const uint8_t* jpg_start, const uint8_t* jpg_end, dl::image::img_t &img);

esp_err_t run_model(ESPDetDetect* detect, dl::image::img_t img);

esp_err_t save_image_as_jpeg(dl::image::img_t &img, const char *filepath);



// Processing BBox for I2C transmission

typedef struct {
    uint8_t object_angle;
    bool direction; // 0 = left, 1 = right 
} object_direction;

object_direction normalise_bbox(dl::detect::result_t bbox);
