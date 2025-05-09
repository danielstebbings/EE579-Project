#include "run_model.hpp"
#include "i2c_handling.h"

#include "driver/i2c_types.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_log_timestamp.h"



static const char *TAG = "run_model";
uint8_t *snapshot_buf = (uint8_t *)malloc(CAM_WIDTH * CAM_HEIGHT * CAM_PIX_BYTES); // points to the output of the capture

esp_err_t camera_init() {
    ESP_LOGI(TAG, "camera_init");
    camera_config_t config;

    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;

    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;

    // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    config.xclk_freq_hz = 20000000;
    config.ledc_timer = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;

    config.pixel_format = PIXFORMAT_JPEG; // YUV422;GRAYSCALE;RGB565;JPEG
    config.frame_size = FRAMESIZE_VGA;   // QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    config.jpeg_quality = 10; // 0-63 lower number means higher quality
    config.fb_count = 1;      // if more than one; i2s runs in continuous modeconfig. Use only with JPEG
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

    // initialize the camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }
    return ESP_OK;
};

esp_err_t camera_capture(dl::image::img_t &img) {
    ESP_LOGI(TAG, "camera_capture");
    // acquire a frame
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera Capture Failed");
        return ESP_FAIL;
    };

     
    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);

    img.data     = snapshot_buf;
    img.width    = CAM_WIDTH;
    img.height   = CAM_HEIGHT;
    img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;

    // return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);

    return ESP_OK;
};

esp_err_t decode_harcoded_jpeg(const uint8_t* jpg_start, const uint8_t* jpg_end, dl::image::img_t &img) {
    ESP_LOGI(TAG, "Beginning Image Decoding");
    dl::image::jpeg_img_t jpg_img = {
        .data   = (uint8_t *)jpg_start,
        .width  = CAM_WIDTH,
        .height = CAM_HEIGHT,
        .data_size = (uint32_t)(jpg_start - jpg_end),
    };

    img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;
    sw_decode_jpeg(jpg_img, img, true);
    ESP_LOGI(TAG, "Decoded image");
    return ESP_OK;
};

esp_err_t run_model(ESPDetDetect* detect, dl::image::img_t img, std::array<dl::detect::result_t,4> &detect_arr) {
    ESP_LOGI(TAG, "Starting Inference");
    uint32_t inference_start = esp_log_timestamp(); // returns ms
    std::list<dl::detect::result_t> &detect_results = detect->run(img);
    int32_t inference_end = esp_log_timestamp(); // returns ms
    ESP_LOGI(TAG, "Finished Inference After %d ms", inference_end - inference_start);

    // Store best classification
    dl::detect::result_t best_can;
    best_can.score = 0.0;
    best_can.category = -1; // To detect if unclassified
    //dl::detect::result_t best_car;
    //best_car.score = 0.0;
    //best_car.category = -1; // To detect if unclassified

    // reinitialize detect_arr elements
    for (auto &result : detect_arr) {
        result = dl::detect::result_t();
        result.score = 0.0; // Ensure that this always exists so that we can check later
    };

    int result_count = 0;
    for (const dl::detect::result_t &res : detect_results) {
        ESP_LOGW(TAG,
                 "[category: %d, score: %f, x1: %d, y1: %d, x2: %d, y2: %d]",
                 res.category,
                 res.score,
                 res.box[0],
                 res.box[1],
                 res.box[2],
                 res.box[3]);

        // Keep track of 4 highest results
        if (result_count < 4) {
            // Less than 4 logged already, space in buffer
            ESP_LOGI(TAG, "Storing Result");
            detect_arr[result_count] = res;
        } else {
            // No space in buffer, have to replace something
            ESP_LOGI(TAG, "More than 4 results, replacing first lowest");
            // iterate through stored results, find first that has a lower score and replace
            for (dl::detect::result_t &stored_result : detect_arr) {
                if (stored_result.score < res.score) {
                    ESP_LOGI(TAG, "Replacing Result");
                    stored_result = res;
                    break;
                } else {
                    // do not replace if equal or higher
                    continue;
                }
            };
        };
        result_count++;
    };

    // detect_arr is now populated

    return ESP_OK;
};

// Processing BBox for I2C transmission
object_direction normalise_bbox(dl::detect::result_t &bbox) {
    ESP_LOGI(TAG, "normalise_bbox");

    object_direction direction;
    // clip boxes to image boundary
    bbox.limit_box(MODEL_WIDTH, MODEL_HEIGHT);
    int box_center = bbox.box[2] - bbox.box[0];

    int img_center = MODEL_WIDTH / 2;

    int norm_center;

    if (box_center >= img_center) {
        // Right side of image
        direction.direction = true;

        // normalise to 0->255, positive angle
        norm_center = (box_center - img_center) * 255;
        norm_center = (2*norm_center) / (MODEL_WIDTH);


    } else {
        // Left side of image
        direction.direction = false;

        // normalise to 0-255, negative angle
        norm_center  = (box_center) * 255;


    }
    norm_center = (2*norm_center) / (MODEL_WIDTH);
    direction.object_angle = norm_center;

    ESP_LOGI(TAG, "norm_Center %d", norm_center);

    return direction;

};

esp_err_t tx_results(std::array<dl::detect::result_t,4> &detect_arr, i2c_master_dev_handle_t dev_handle) {

    std::array<object_direction,4> object_directions;

    // convert result_t to directions
    uint8_t object_count = 0;
    for (dl::detect::result_t &res : detect_arr) {
        // only add to array if it actually contains a detection
        if (res.score != 0.0) {
            object_directions[object_count] = normalise_bbox(res);
            ESP_LOGI(TAG, "post_norm: d %d, a %d", object_directions[object_count].direction, object_directions[object_count].object_angle);
            object_count++;
        } else {
            continue;
        }
        
    }
    ESP_LOGI(TAG, "Constructing I2C Packet");
    // populate I2C Packet
    // packet[0] -> Header
    // packet[1..4] -> Object_1 -> 4
    uint8_t packet[5] = {0};
    // Detection Header
    // - - bit 0:3  - Amount of objects detected, 0 -> 4
    // - - bit 4    - Object 1 left / right flag
    // - - bit 5    - Object 2 left / right flag
    // - - bit 6    - Object 3 left / right flag
    // - - bit 7    - Object 4 left / right flag
    packet[0] = object_count;
    int flag_bit_pos;
    uint8_t flag;
    for (int it = 0; it < object_count; it++) {
        flag_bit_pos = 4 + it;
        flag = object_directions[it].direction << flag_bit_pos;
        packet[0] = packet[0] || flag;
        packet[it+1] = object_directions[it].object_angle;
    }

    ESP_LOGI(TAG, "Transmitting I2C Packet with %d objects", object_count);
    ESP_LOGI(TAG, "PKT: H %02X | C0 %02X | C1 %02X | C2 %02X | C3 %02X |", packet[0],packet[1],packet[2],packet[3],packet[4]);
    if (register_write_bytes(dev_handle,ESP_ADDR, packet) != ESP_OK) {
        ESP_LOGE(TAG, "Packet transmission failed");
    }

    return ESP_OK;

};