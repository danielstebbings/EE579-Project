#include "coco_detect.hpp"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

extern const uint8_t bus_jpg_start[] asm("_binary_bus_jpg_start");
extern const uint8_t bus_jpg_end[] asm("_binary_bus_jpg_end");
const char *TAG = "yolo11n";

extern "C" void app_main(void)
{

    dl::image::jpeg_img_t jpeg_img = {
        .data = (uint8_t *)bus_jpg_start,
        .width = 320,
        .height = 320,
        .data_size = (uint32_t)(bus_jpg_end - bus_jpg_start),
    };
    dl::image::img_t img;
    img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;
    sw_decode_jpeg(jpeg_img, img, true);
    ESP_LOGI(TAG, "Decoded image");

    COCODetect *detect = new COCODetect();
    auto &detect_results = detect->run(img);
    ESP_LOGI(TAG, "Finished Inference");
    for (const auto &res : detect_results) {
        ESP_LOGI(TAG,
                 "[category: %d, score: %f, x1: %d, y1: %d, x2: %d, y2: %d]",
                 res.category,
                 res.score,
                 res.box[0],
                 res.box[1],
                 res.box[2],
                 res.box[3]);
    }
    delete detect;
    heap_caps_free(img.data);
}
