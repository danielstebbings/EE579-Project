#pragma once
#include "dl_detect_base.hpp"
#include "dl_detect_espdet_postprocessor.hpp"

namespace espdet_detect {
class ESPDet : public dl::detect::DetectImpl {
public:
    ESPDet(const char *model_name);
};
} // namespace espdet_detect

class ESPDetDetect : public dl::detect::DetectWrapper {
public:
    typedef enum {
        ESPDET_PICO_416_416_CAN,
    } model_type_t;
    ESPDetDetect(model_type_t model_type = static_cast<model_type_t>(CONFIG_DEFAULT_ESPDET_DETECT_MODEL_TYPE));
};
