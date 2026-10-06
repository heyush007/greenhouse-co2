// Shared message types the tasks pass through the queues.
#pragma once
#include <cstdint>
#include "FreeRTOS.h"
#include "queue.h"

// Latest sensor readings (Modbus task -> Control task).
struct SensorData {
    int16_t    co2_ppm;
    float      rh_pct;
    float      temp_c;
    uint16_t   fan_pulses;     // fan rotation pulses since the last read
    bool       fan_running;    // false after two zero reads in a row
    bool       co2_ok;         // last CO2 read worked
    bool       rh_t_ok;        // last RH/T read worked
    bool       mio_ok;         // last fan I/O read worked
    TickType_t timestamp;      // when these values were read
};

// Fan speed request (Control task -> Modbus task).
struct FanCommand {
    uint8_t percent;
};

// Everything the UI and network show (Control task -> UI, Network).
struct SystemStatus {
    SensorData sensors;
    uint16_t   setpoint_ppm;
    uint8_t    fan_percent;
    bool       valve_open;
    bool       venting;        // true while venting above the safety limit
};

// A new CO2 target (UI, Network -> Control task).
struct SetpointRequest {
    uint16_t ppm;
};

// One button or encoder event (input ISR -> UI task).
enum class InputEvent : uint8_t {
    RotCW, RotCCW, RotPress, Sw0, Sw1, Sw2
};

// Which setting to change (UI, Network, Console -> Storage task).
enum class SaveField : uint8_t { Setpoint, WifiSsid, WifiPassword, ThingSpeakKey,
                                 TalkbackKey, TalkbackId, FactoryReset };
// One change request for the Storage task.
struct SaveRequest {
    SaveField field;
    uint16_t  ppm;             // used for Setpoint
    char      text[65];        // used for the string settings
};

// Live network status for the UI's WiFi screen (Network -> UI).
struct NetStatus {
    bool connected;
    char ip[16];
};

// Everything kept in EEPROM.
struct Settings {
    uint32_t magic;            // marks a valid saved block
    uint16_t version;
    uint16_t setpoint_ppm;
    char     wifi_ssid[33];
    char     wifi_password[65];
    char     thingspeak_api_key[17];
    char     thingspeak_talkback_key[17];
    char     thingspeak_talkback_id[12];
    uint32_t crc;              // checksum over everything above
};

constexpr uint32_t SETTINGS_MAGIC   = 0x47484331;
constexpr uint16_t SETTINGS_VERSION = 2;
