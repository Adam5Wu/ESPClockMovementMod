#include "Module.hpp"

#include <string>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "uart.h"

#include "ZWUtils.hpp"
#include "ZWAppUtils.hpp"

#include "AppConfig/Interface.hpp"
#include "AppEventMgr/Interface.hpp"
#include "AppHTTPD/Interface.hpp"

#include "Config.hpp"
#include "HTTPD_Handler.hpp"

namespace zw::esp8266::app::wificlock {
namespace {

inline constexpr char TAG[] = "WiFiClock";

inline constexpr TickType_t CLOCK_TASK_INTERVAL = CONFIG_FREERTOS_HZ * 3;  // Every 3 sec
inline constexpr char CLOCK_TIME_TEMPLATE[] = "Clock device -->\n+TIME:%s %02d:%02d";
inline constexpr char CLOCK_TIME_PATTERN[] = "%c %Z";

using config::AppConfig;

Config config_;

void _clock_task(TimerHandle_t) {
  if (eventmgr::system_states_peek(ZW_SYSTEM_STATE_TIME_NTP_TRACKING)) {
    struct timeval cur_time;
    if (gettimeofday(&cur_time, NULL) != 0) {
      ESP_LOGW(TAG, "Unable to get current time");
      return;
    }
    // Account for manual offset
    time_t disp_time_sec = cur_time.tv_sec + config_.time_offset * 60;

    struct tm time_tm;
    if (localtime_r(&disp_time_sec, &time_tm) == NULL) {
      ESP_LOGW(TAG, "Failed to convert time to local");
      return;
    }
    char time_str[32];
    if (strftime(time_str, 32, CLOCK_TIME_PATTERN, &time_tm) == 0) {
      ESP_LOGW(TAG, "Failed to format time");
      return;
    }

    uint16_t resync_at = config_.resync_time;
    // Use the most critical log level to ensure always print
    ESP_LOG_LEVEL(ESP_LOG_NONE, TAG, CLOCK_TIME_TEMPLATE, time_str, resync_at / 60, resync_at % 60);
  }
}

TimerHandle_t clock_task_handle_ = NULL;

void _init_clock_task(int32_t event_id, void*, void*) {
  switch (event_id) {
    case ZW_SYSTEM_EVENT_NET_STA_IP_READY:
      if (clock_task_handle_ == NULL) {
        ESP_LOGD(TAG, "Starting clock task...");
        clock_task_handle_ = xTimerCreate("zw_wifi_clock", CLOCK_TASK_INTERVAL, pdTRUE, NULL,
                                          ZWTimerWrapper<TAG, _clock_task>);
        if (clock_task_handle_ == NULL) {
          ESP_LOGE(TAG, "Failed to create clock timer task");
          eventmgr::SetSystemFailed();
          return;
        }
        if (xTimerStart(clock_task_handle_, CONFIG_FREERTOS_HZ) != pdPASS) {
          xTimerDelete(clock_task_handle_, CONFIG_FREERTOS_HZ);
          ESP_LOGE(TAG, "Failed to start clock timer task");
          eventmgr::SetSystemFailed();
          return;
        }
      }
      break;

    case ZW_SYSTEM_EVENT_TIME_NTP_TRACKING:
      _clock_task(NULL);
      break;

    default:
      ESP_LOGW(TAG, "Unrecognized event %d", event_id);
  }

  eventmgr::system_event_unregister_handler(
      event_id, eventmgr::SystemEventHandlerWrapper<TAG, _init_clock_task>);
  return;
}

esp_err_t _init_wificlock() {
  config_ = config::get()->wificlock;

  // Add HTTPD handler registrar
  httpd::add_ext_handler_registrar(register_httpd_handler);

  ESP_RETURN_ON_ERROR(eventmgr::system_event_register_handler(
      ZW_SYSTEM_EVENT_NET_STA_IP_READY,
      eventmgr::SystemEventHandlerWrapper<TAG, _init_clock_task>));
  ESP_RETURN_ON_ERROR(eventmgr::system_event_register_handler(
      ZW_SYSTEM_EVENT_TIME_NTP_TRACKING,
      eventmgr::SystemEventHandlerWrapper<TAG, _init_clock_task>));
  return ESP_OK;
}

Config& _get_config(AppConfig& config) { return config.wificlock; }

}  // namespace

utils::ESPErrorStatus Update_Config(Config&& config) {
  config_ = std::move(config);

  {
    auto new_config = config::get();
    new_config->wificlock = config_;
    ESP_RETURN_ON_ERROR(config::persist());
  }

  return ESP_OK;
}

esp_err_t config_init(void) {
  ESP_LOGD(TAG, "Initializing for config...");
  return config::register_custom_field("wificlock", _get_config, parse_config, log_config,
                                       marshal_config);
}

esp_err_t init(void) {
  ESP_LOGD(TAG, "Initializing...");
  ESP_RETURN_ON_ERROR(_init_wificlock());
  return ESP_OK;
}

void finit(void) {
  // Nothing
}

}  // namespace zw::esp8266::app::wificlock
