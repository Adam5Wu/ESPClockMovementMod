#include "Config.hpp"

#include "esp_log.h"

#include "cJSON.h"

#include "ZWUtils.hpp"

#include "AppConfig/Interface.hpp"
#include "Interface.hpp"

namespace zw::esp8266::app::wificlock {
namespace {

inline constexpr char TAG[] = "WiFiClock-Config";

//-------------------------
// Config handling macros

#define PARSE_AND_ASSIGN_FIELD(json, container, field, parse_func, strict) \
  ESP_RETURN_ON_ERROR(                                                     \
      config::parse_and_assign_field(json, #field, container.field, parse_func, strict))

#define DIFF_AND_MARSHAL_FIELD(container, base, update, field, marshal_func) \
  ESP_RETURN_ON_ERROR(                                                       \
      config::diff_and_marshal_field(container, #field, base.field, update.field, marshal_func))

std::string _print_time(uint16_t time) {
  if (time > 24 * 60) return "(invalid time)";
  return utils::DataBuf(6).PrintTo("%02d:%02d", time / 60, time % 60);
}

}  // namespace

esp_err_t parse_config(const cJSON* json, Config& container, bool strict) {
  PARSE_AND_ASSIGN_FIELD(json, container, time_offset,
                         (config::string_decoder<int16_t, config::decode_short_int>), strict);
  PARSE_AND_ASSIGN_FIELD(json, container, resync_time,
                         (config::string_decoder<uint16_t, config::decode_short_size>), strict);

  return ESP_OK;
}

void log_config(const Config& config) {
  ESP_LOGI(TAG, "- WiFi clock config:");
  if (config.time_offset) {
    ESP_LOGI(TAG, "  Time offset: %dmin", config.time_offset);
  }
  ESP_LOGI(TAG, "  Daily re-sync at: %s", _print_time(config.resync_time).c_str());
}

esp_err_t marshal_config(utils::AutoReleaseRes<cJSON*>& container, const Config& base,
                         const Config& update) {
  DIFF_AND_MARSHAL_FIELD(container, base, update, time_offset,
                         (config::string_encoder<int16_t, config::encode_short_int>));
  DIFF_AND_MARSHAL_FIELD(container, base, update, resync_time,
                         (config::string_encoder<uint16_t, config::encode_short_size>));
  return ESP_OK;
}

}  // namespace zw::esp8266::app::wificlock
