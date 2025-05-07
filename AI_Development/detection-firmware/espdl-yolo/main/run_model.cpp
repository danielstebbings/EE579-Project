#include "run_model.hpp"

#include "esp_camera.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "cam_handling";
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
    config.frame_size = FRAMESIZE_320X320;   // QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    config.jpeg_quality = 10; // 0-63 lower number means higher quality
    config.fb_count = 2;      // if more than one; i2s runs in continuous modeconfig. Use only with JPEG
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

esp_err_t run_model(COCODetect* detect, dl::image::img_t img) {
    ESP_LOGI(TAG, "Starting Inference");
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
    };

    return ESP_OK;
};

esp_err_t save_image_as_jpeg(dl::image::img_t &img, const char *filepath) {
    jpeg_enc_cfg_t jpeg_cfg = {
        .width      = img.width,
        .height     = img.height,
        .src_type   = JPEG_RAW_TYPE_RGB888,
        .quality    = 90,
    };

     // Estimate max output size: width * height / 2 is usually safe for JPEG
     size_t max_jpeg_size = width * height / 2;
     uint8_t *jpeg_buf = (uint8_t *)heap_caps_malloc(max_jpeg_size, MALLOC_CAP_SPIRAM);
     if (!jpeg_buf) {
         ESP_LOGE(TAG, "Failed to allocate JPEG buffer");
         return ESP_ERR_NO_MEM;
     }

     jpeg_enc_output_t out_buf = {
        .buf = jpeg_buf,
        .buf_size = max_jpeg_size,
        .out_size = 0,
    };

    ret = esp_jpeg_encode(&jpeg_cfg, rgb888_data, &out_buf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "JPEG encoding failed");
        heap_caps_free(jpeg_buf);
        return ret;
    }

    // Write to file
    FILE *f = fopen(filepath, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file %s for writing", filepath);
        heap_caps_free(jpeg_buf);
        return ESP_FAIL;
    }

    size_t written = fwrite(out_buf.buf, 1, out_buf.out_size, f);
    fclose(f);
    heap_caps_free(jpeg_buf);

    if (written != out_buf.out_size) {
        ESP_LOGE(TAG, "Failed to write complete JPEG file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Saved JPEG to %s (%d bytes)", filepath, (int)written);
    return ESP_OK;

}
