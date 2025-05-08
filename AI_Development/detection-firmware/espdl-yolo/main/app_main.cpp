#include "espdet_detect.hpp"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

#include "run_model.hpp"

#include "driver/temperature_sensor.h"

extern const uint8_t xiao_vga_calib_jpg_start[] asm("_binary_xiao_vga_calib_jpg_start");
extern const uint8_t xiao_vga_calib_jpg_end[] asm("_binary_xiao_vga_calib_jpg_end");
static const char *TAG = "yolo11n";



void check_psram_status() {
    size_t total_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

    ESP_LOGI("PSRAM", "Total PSRAM: %d bytes", total_psram);
    ESP_LOGI("PSRAM", "Free PSRAM: %d bytes", free_psram);
    ESP_LOGI("PSRAM", "Largest Free Block: %d bytes", largest_block);
}
extern "C" void app_main(void) {
    check_psram_status();
    ESP_LOGI(TAG, "app_main");
    ESPDetDetect* detect = new ESPDetDetect();
    dl::image::img_t img;
    uint8_t *resized_buf = (uint8_t *)malloc(MODEL_WIDTH * MODEL_HEIGHT * CAM_PIX_BYTES);

    
    ESP_LOGI(TAG, "Decoding Hardcoded Image");
    decode_harcoded_jpeg(xiao_vga_calib_jpg_start, xiao_vga_calib_jpg_end, img);

    if(MODEL_WIDTH != CAM_WIDTH || MODEL_HEIGHT != CAM_HEIGHT) {
        ESP_LOGI(TAG, "Resizing Hardcoded Image");
        dl::image::img_t res_img;
        res_img.data        = resized_buf;
        res_img.width       = MODEL_WIDTH;
        res_img.height      = MODEL_HEIGHT;
        res_img.pix_type    = dl::image::DL_IMAGE_PIX_TYPE_RGB888;

        dl::image::resize(
            img,
            res_img,
            dl::image::DL_IMAGE_INTERPOLATE_BILINEAR,  // or DL_IMAGE_INTERPOLATE_NEAREST
            0,              // caps: 0 if you don’t need RGB/BGR swap
            nullptr,        // norm_lut: optional normalization
            {}              // crop_area: empty = full image
        );

        run_model(detect, res_img);
    } else {
        run_model(detect, img);
    }

    ESP_LOGI(TAG, "Decoding Camera Output");
    camera_init();
    while(true) {
        if (camera_capture(img) != ESP_OK) {
            ESP_LOGW(TAG, "Camera Capture Failed: Skipping Inference");
        } else {
            ESP_LOGI(TAG, "Successful Capture");
            if(MODEL_WIDTH != CAM_WIDTH || MODEL_HEIGHT != CAM_HEIGHT) {
                ESP_LOGI(TAG, "Resizing");
                dl::image::img_t res_img;
                res_img.data        = resized_buf;
                res_img.width       = MODEL_WIDTH;
                res_img.height      = MODEL_HEIGHT;
                res_img.pix_type    = dl::image::DL_IMAGE_PIX_TYPE_RGB888;

                dl::image::resize(
                    img,
                    res_img,
                    dl::image::DL_IMAGE_INTERPOLATE_BILINEAR,  // or DL_IMAGE_INTERPOLATE_NEAREST
                    0,              // caps: 0 if you don’t need RGB/BGR swap
                    nullptr,        // norm_lut: optional normalization
                    {}              // crop_area: empty = full image
                );

                run_model(detect, res_img);
            } else {
                run_model(detect, img);
            }
            
        }
        
    }

    
    return;

}