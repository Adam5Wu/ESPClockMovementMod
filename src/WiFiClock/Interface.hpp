
#ifndef APP_WIFICLOCK_INTERFACE
#define APP_WIFICLOCK_INTERFACE

#include <stdint.h>

namespace zw::esp8266::app::wificlock {

struct Config {
  // Apply an offset to the display time
  int16_t time_offset = 0;

  // Daily re-sync time (default at 6:00AM)
  uint16_t resync_time = 6 * 60;
};

}  // namespace zw::esp8266::app::wificlock

#endif  // APP_WIFICLOCK_INTERFACE
