// Storage task: owns the EEPROM and the single copy of the settings.
#include <cstdio>
#include <cstring>
#include "tasks.h"
#include "config.h"
#include "app_queues.h"
#include "settings.h"
#include "FreeRTOS.h"
#include "task.h"

namespace {

// Copy a string safely, always leaving it terminated.
void copy_text(char *dst, size_t size, const char *src) {
    size_t i = 0;
    for (; i + 1 < size && src[i] != '\0'; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

// Apply one change to the settings.
void apply(Settings &s, const SaveRequest &r) {
    switch (r.field) {
        case SaveField::Setpoint:
            s.setpoint_ppm = r.ppm > cfg::SETPOINT_MAX_PPM ? cfg::SETPOINT_MAX_PPM : r.ppm;
            break;
        case SaveField::WifiSsid:      copy_text(s.wifi_ssid, sizeof s.wifi_ssid, r.text); break;
        case SaveField::WifiPassword:  copy_text(s.wifi_password, sizeof s.wifi_password, r.text); break;
        case SaveField::ThingSpeakKey: copy_text(s.thingspeak_api_key, sizeof s.thingspeak_api_key, r.text); break;
        case SaveField::TalkbackKey:   copy_text(s.thingspeak_talkback_key, sizeof s.thingspeak_talkback_key, r.text); break;
        case SaveField::TalkbackId:    copy_text(s.thingspeak_talkback_id, sizeof s.thingspeak_talkback_id, r.text); break;
        case SaveField::FactoryReset:  s = settings_defaults(); break;
    }
    settings_seal(s);
}

}

void storage_task(void *param) {
    Settings settings = *static_cast<Settings *>(param);   // start from the boot copy

    while (true) {
        // Wait for a change request.
        SaveRequest req;
        if (xQueueReceive(saveQueue, &req, portMAX_DELAY) != pdTRUE) continue;

        apply(settings, req);
        // Merge any quick follow-up changes into one write.
        while (xQueueReceive(saveQueue, &req, pdMS_TO_TICKS(200)) == pdTRUE) apply(settings, req);

        const bool ok = settings_save(settings);
        printf("[storage] settings %s (SP %u ppm)\n", ok ? "saved" : "SAVE FAILED", settings.setpoint_ppm);
    }
}
