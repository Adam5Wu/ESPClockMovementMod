#include "HTTPD_Handler.hpp"

#include "esp_http_server.h"
#include "esp_log.h"

#include "cJSON.h"

#include "ZWUtils.hpp"

#include "AppConfig/Interface.hpp"
#include "AppHTTPD/Interface.hpp"

#include "Interface_Private.hpp"

namespace zw::esp8266::app::wificlock {
namespace {

inline constexpr char TAG[] = "TWiLight-HTTPD";

inline constexpr char URI_PATTERN[] = "/!wificlock*";
#define URI_PATH_DELIM '/'

esp_err_t _get_config(httpd_req_t* req) {
  Config config = config::get()->wificlock;

  utils::AutoReleaseRes<cJSON*> config_json(cJSON_CreateObject(), cJSON_Delete);
  if (marshal_config(config_json, {}, config) != ESP_OK) {
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to marshal config");
  }
  return httpd::send_json(req, *config_json);
}

esp_err_t _put_config(httpd_req_t* req) {
  utils::AutoReleaseRes<cJSON*> json;
  if (httpd::receive_json(req, json) != ESP_OK) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive config data");
  }

  Config new_config = config::get()->wificlock;
  if (parse_config(*json, new_config, true) != ESP_OK) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid config data");
  }

  auto result = Update_Config(std::move(new_config));
  if (!result) {
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, result.message.c_str());
  }

  return httpd_resp_send(req, NULL, 0);
}

esp_err_t _handler_wificlock(httpd_req_t* req) {
  ESP_LOGI(TAG, "[%s] %s", http_method_str((enum http_method)req->method), req->uri);
  const char* feature = req->uri + utils::STRLEN(URI_PATTERN) - 1;
  if (*feature != '\0') {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Malformed request");
  }

  switch (req->method) {
    case HTTP_GET:
      return _get_config(req);
    case HTTP_PUT:
      return _put_config(req);
    default:
      return httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Method not allowed");
  }
}

}  // namespace

esp_err_t register_httpd_handler(httpd_handle_t httpd) {
  ESP_LOGD(TAG, "Register handler on %s", URI_PATTERN);
  httpd_uri_t handler = {
      .uri = URI_PATTERN,
      .method = HTTP_ANY,
      .handler = _handler_wificlock,
      .user_ctx = NULL,
  };
  return httpd_register_uri_handler(httpd, &handler);
}

}  // namespace zw::esp8266::app::wificlock
