#include "cam_handling.hpp"

#include "esp_camera.h"
#include "esp_log.h"

// EI includes
#include "model-parameters/model_metadata.h"
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
static const char *TAG = "cam_handling";

uint8_t *snapshot_buf; // points to the output of the capture

esp_err_t camera_init() {
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
    config.frame_size = FRAMESIZE_QVGA;   // QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    config.jpeg_quality = 12; // 0-63 lower number means higher quality
    config.fb_count = 1;      // if more than one; i2s runs in continuous modeconfig. Use only with JPEG
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    // initialize the camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ////ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }
    return ESP_OK;
};

esp_err_t camera_capture() {

    // acquire a frame
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        //ESP_LOGE(TAG, "Camera Capture Failed");
        return ESP_FAIL;
    }
    // replace this with your own function
    // process_image(fb->width, fb->height, fb->format, fb->buf, fb->len);

    snapshot_buf = (uint8_t *)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
    // return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);
    
    ei::image::processing::resize_image_using_mode(
            snapshot_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            snapshot_buf,
            EI_CLASSIFIER_INPUT_WIDTH,
            EI_CLASSIFIER_INPUT_HEIGHT,
            3, // 3 Bytes per pixel
            EI_CLASSIFIER_RESIZE_SQUASH  // SQUASH
        );

    return ESP_OK;
}

int feature_get_data(size_t offset, size_t length, float* out_ptr) {
    // https://github.com/edgeimpulse/firmware-espressif-esp32/blob/main/edge-impulse/inference/ei_run_camera_impulse.cpp#L63
    // we already have a RGB888 buffer, so recalculate offset into pixel index
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;

    while (pixels_left != 0) {
        out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix + 2];

        // go to the next pixel
        out_ptr_ix++;
        pixel_ix+=3;
        pixels_left--;
    }

    // and done!
    return 0;
}

EI_IMPULSE_ERROR run_model(const ei_impulse_t* impulse, signal_t* image, ei_impulse_result_t* result) {
    EI_IMPULSE_ERROR classifier_error = run_classifier_image_quantized(impulse, image, result, true);
    if (classifier_error != EI_IMPULSE_OK) {
        //ESP_LOGE(TAG, "Classifier Failed");
        return EI_IMPULSE_INFERENCE_ERROR;
    } else {
        ESP_LOGI(TAG, "Detected: %li Objects ", result->bounding_boxes_count);
        return EI_IMPULSE_OK;
    }
}

EI_IMPULSE_ERROR detect(ei_impulse_result_t* result) {
    // Take photo
    esp_err_t cap_error = camera_capture();
    if (cap_error != ESP_OK) {
        //ESP_LOGE(TAG,"Capture Failed");
        return EI_IMPULSE_DSP_ERROR;
    } else{
        //ESP_LOGI(TAG,"Successful capture");
    }
    
    // Pass to model
    signal_t image;
    // Width * height * 3 Bytes per pixel
    image.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * 3;
    image.get_data = static_cast<int(*)(size_t, size_t, float*)>(&feature_get_data); // Solving unresolved overload errors by static cast, What could go wrong?

    EI_IMPULSE_ERROR model_error = run_model(&impulse, &image, result);
    if (model_error != EI_IMPULSE_OK) {
        //ESP_LOGE(TAG,"Model Run failed");
        return model_error;
    } else {
        //ESP_LOGI(TAG, "Model Run Succesful");
        return EI_IMPULSE_OK;
    }
}



