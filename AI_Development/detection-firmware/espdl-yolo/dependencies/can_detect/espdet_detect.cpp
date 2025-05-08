#include "espdet_detect.hpp"
#include "esp_log.h"

#if CONFIG_ESPDET_DETECT_MODEL_IN_FLASH_RODATA
extern const uint8_t can_detect_espdl[] asm("_binary_can_detect_espdl_start");
static const char *path = (const char *)can_detect_espdl;
#elif CONFIG_ESPDET_DETECT_MODEL_IN_FLASH_PARTITION
static const char *path = "espdet_det";
#else
#if !defined(CONFIG_BSP_SD_MOUNT_POINT)
#define CONFIG_BSP_SD_MOUNT_POINT "/sdcard"
#endif
#endif
namespace espdet_detect {
ESPDet::ESPDet(const char *model_name)
{
#if !CONFIG_ESPDET_DETECT_MODEL_IN_SDCARD
    m_model =
        new dl::Model(path, model_name, static_cast<fbs::model_location_type_t>(CONFIG_ESPDET_DETECT_MODEL_LOCATION));
#else
    char sd_path[256];
    snprintf(sd_path,
             sizeof(sd_path),
             "%s/%s/%s",
             CONFIG_BSP_SD_MOUNT_POINT,
             CONFIG_ESPDET_DETECT_MODEL_SDCARD_DIR,
             model_name);
    m_model = new dl::Model(sd_path, static_cast<fbs::model_location_type_t>(CONFIG_ESPDET_DETECT_MODEL_LOCATION));
#endif
    m_model->minimize();
#if CONFIG_IDF_TARGET_ESP32P4
    m_image_preprocessor =
        new dl::image::ImagePreprocessor(m_model, {0, 0, 0}, {255, 255, 255}, DL_IMAGE_CAP_RGB565_BIG_ENDIAN);
#else
    m_image_preprocessor = new dl::image::ImagePreprocessor(m_model, {0, 0, 0}, {255, 255, 255});
#endif
    m_postprocessor =
        new dl::detect::ESPDetPostProcessor(m_model, 0.25, 0.7, 10, {{8, 8, 4, 4}, {16, 16, 8, 8}, {32, 32, 16, 16}});
}

} // namespace espdet_detect

ESPDetDetect::ESPDetDetect(model_type_t model_type)
{
    switch (model_type) {
    case model_type_t::ESPDET_PICO_416_416_CAN:
#if CONFIG_ESPDET_PICO_416_416_CAN || CONFIG_CAN_DETECT_MODEL_IN_SDCARD
        m_model = new espdet_detect::ESPDet("espdet_pico_416_416_can.espdl");
#else
        ESP_LOGE("can_detect", "espdet_pico_416_416_can is not selected in menuconfig.");
#endif
        break;
    }
}
