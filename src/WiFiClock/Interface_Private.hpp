
#ifndef APP_WIFICLOCK_INTERFACE_PRIVATE
#define APP_WIFICLOCK_INTERFACE_PRIVATE

#include "esp_err.h"
#include "cJSON.h"

#include "ZWUtils.hpp"

#include "Interface.hpp"

namespace zw::esp8266::app::wificlock {

extern esp_err_t parse_config(const cJSON* json, Config& container, bool strict);
extern esp_err_t marshal_config(utils::AutoReleaseRes<cJSON*>& container, const Config& base,
                                const Config& update);

extern utils::ESPErrorStatus Update_Config(Config&& config);

}  // namespace zw::esp8266::app::wificlock

#endif  // APP_WIFICLOCK_INTERFACE_PRIVATE
