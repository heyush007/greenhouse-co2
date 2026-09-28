// The contract between tasks. Change this file only through a merge request
// that everyone has seen: every task depends on it.
#pragma once
#include <cstdint>
#include "FreeRTOS.h"
#include "queue.h"

// Modbus task -> Control task (mailbox: sensorData)
struct SensorData {
    int16_t    co2_ppm;
    float      rh_pct;
    float      temp_c;
    uint16_t   fan_pulses;     // pulses since last read (MIO counter self-clears)
    bool       fan_running;    // false after two consecutive zero reads
    bool       co2_ok;         // last GMP252 read succeeded
    bool       rh_t_ok;        // last HMP60 read succeeded
    bool       mio_ok;         // last MIO access succeeded
    TickType_t timestamp;      // xTaskGetTickCount() when the data was read
};

// Control task -> Modbus task (queue: fanCmdQueue)
struct FanCommand {
    uint8_t percent;           // 0..100
};

// Control task -> UI, Network (mailbox: systemStatus)
struct SystemStatus {
    SensorData sensors;
    uint16_t   setpoint_ppm;
    uint8_t    fan_percent;
    bool       valve_open;
    bool       venting;        // CO2 went above the 2000 ppm safety limit
};

// UI, Network -> Control (queue: setpointQueue)
struct SetpointRequest {
    uint16_t ppm;
};

// GPIO ISR -> UI task (queue: inputQueue)
enum class InputEvent : uint8_t {
    RotCW, RotCCW, RotPress, Sw0, Sw1, Sw2
};

// UI, Network -> Storage task (queue: saveQueue)
// Senders describe one change; the Storage task owns the full Settings copy and merges it.
// That keeps Settings out of shared memory, so no mutex is needed.
enum class SaveField : uint8_t { Setpoint, WifiSsid, WifiPassword, ThingSpeakKey, FactoryReset };
struct SaveRequest {
    SaveField field;
    uint16_t  ppm;             // used with Setpoint
    char      text[65];        // used with the string fields
};

// What lives in EEPROM (owned by the Storage task after boot)
struct Settings {
    uint32_t magic;            // SETTINGS_MAGIC when valid
    uint16_t version;
    uint16_t setpoint_ppm;
    char     wifi_ssid[33];
    char     wifi_password[65];
    char     thingspeak_api_key[17];
    uint32_t crc;              // CRC32 over everything above
};

constexpr uint32_t SETTINGS_MAGIC   = 0x47484331; // "GHC1"
constexpr uint16_t SETTINGS_VERSION = 1;
