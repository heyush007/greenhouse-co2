// Loading, saving and validating the settings stored in EEPROM.
#include "settings.h"
#include "config.h"
#include "Eeprom.h"
#include <cstring>
#include <cstddef>

namespace {
// Standard CRC32, used to detect corrupted EEPROM data.
uint32_t crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
    }
    return ~crc;
}

// Checksum over every field except the crc itself.
uint32_t settings_crc(const Settings &s) {
    return crc32(reinterpret_cast<const uint8_t *>(&s), offsetof(Settings, crc));
}
}

// Factory defaults with an empty network config.
Settings settings_defaults() {
    Settings s;
    std::memset(&s, 0, sizeof s);
    s.setpoint_ppm = cfg::SETPOINT_DEFAULT_PPM;
    settings_seal(s);
    return s;
}

// Stamp the magic, version and checksum before saving.
void settings_seal(Settings &s) {
    s.magic   = SETTINGS_MAGIC;
    s.version = SETTINGS_VERSION;
    s.wifi_ssid[sizeof s.wifi_ssid - 1] = '\0';
    s.wifi_password[sizeof s.wifi_password - 1] = '\0';
    s.thingspeak_api_key[sizeof s.thingspeak_api_key - 1] = '\0';
    s.thingspeak_talkback_key[sizeof s.thingspeak_talkback_key - 1] = '\0';
    s.thingspeak_talkback_id[sizeof s.thingspeak_talkback_id - 1] = '\0';
    s.crc = settings_crc(s);
}

// True if the block is ours and undamaged.
bool settings_valid(const Settings &s) {
    return s.magic == SETTINGS_MAGIC && s.version == SETTINGS_VERSION &&
           s.setpoint_ppm <= cfg::SETPOINT_MAX_PPM && s.crc == settings_crc(s);
}

// Load settings, or fall back to defaults if blank or invalid.
bool settings_load(Settings &out) {
    Settings s;
    if (eeprom::read(cfg::EEPROM_SETTINGS_ADDR, reinterpret_cast<uint8_t *>(&s), sizeof s) && settings_valid(s)) {
        out = s;
        return true;
    }
    out = settings_defaults();
    return false;
}

// Write the settings block to EEPROM.
bool settings_save(const Settings &s) {
    return eeprom::write(cfg::EEPROM_SETTINGS_ADDR, reinterpret_cast<const uint8_t *>(&s), sizeof s);
}
