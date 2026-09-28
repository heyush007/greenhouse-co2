// Network task (OPTIONAL advanced feature). Disabled with cfg::ENABLE_NETWORK_TASK = false.
//
// TODO(network owner), in this order:
//   1. Build for the Pico W (PICO_BOARD=pico_w) and bring up Wi-Fi with cyw43_arch in
//      threadsafe_background or sys_freertos mode.
//   2. Every 30 s: xQueuePeek(systemStatus) and HTTP POST to ThingSpeak:
//        field1=CO2 ppm, field2=RH %, field3=T C, field4=fan %, field5=setpoint ppm
//   3. Poll the TalkBack queue; for a valid setpoint command send SetpointRequest to
//      setpointQueue and SaveRequest{Setpoint} to saveQueue.
//   4. (Advanced) TLS, and credentials entered from the UI -> SaveRequest{WifiSsid,...}.
#include <cstdio>
#include "tasks.h"
#include "config.h"
#include "app_queues.h"
#include "FreeRTOS.h"
#include "task.h"

void network_task(void *param) {
    const Settings creds = *static_cast<const Settings *>(param);   // read-only snapshot
    printf("[network] not implemented yet (ssid '%s')\n", creds.wifi_ssid);

    while (true) {
        SystemStatus st;
        if (xQueuePeek(systemStatus, &st, 0) == pdTRUE) {
            printf("[network] would send field1=%d&field2=%.1f&field3=%.1f&field4=%u&field5=%u\n",
                   st.sensors.co2_ppm, st.sensors.rh_pct, st.sensors.temp_c, st.fan_percent, st.setpoint_ppm);
        }
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
