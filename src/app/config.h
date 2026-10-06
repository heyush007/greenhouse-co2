// All the pins, addresses and tuning numbers in one place.
#pragma once
#include <cstdint>

namespace cfg {

// Modbus bus (sensors and fan I/O).
constexpr int MODBUS_UART      = 1;
constexpr int MODBUS_TX_PIN    = 4;
constexpr int MODBUS_RX_PIN    = 5;
constexpr int MODBUS_BAUD      = 9600;
constexpr int MODBUS_STOP_BITS = 2;
constexpr int MODBUS_RESPONSE_TIMEOUT_MS = 100;

// Modbus device addresses.
constexpr uint8_t ADDR_MIO    = 1;
constexpr uint8_t ADDR_GMP252 = 240;
constexpr uint8_t ADDR_HMP60  = 241;

// Registers we read and write.
constexpr uint16_t GMP252_CO2_PPM_INT16 = 0x0100;
constexpr uint16_t HMP60_RH_X10         = 0x0100;
constexpr uint16_t HMP60_T_X10          = 0x0101;
constexpr uint16_t MIO_AO1              = 0;
constexpr uint16_t MIO_AI1_COUNTER      = 4;

// GPIO pins for the valve, encoder and buttons.
constexpr unsigned VALVE_PIN   = 27;
constexpr unsigned ROT_A_PIN   = 10;
constexpr unsigned ROT_B_PIN   = 11;
constexpr unsigned ROT_SW_PIN  = 12;
constexpr unsigned SW0_PIN     = 9;
constexpr unsigned SW1_PIN     = 8;
constexpr unsigned SW2_PIN     = 7;

// I2C buses: OLED and pressure on I2C1, EEPROM on I2C0.
constexpr unsigned I2C1_SDA_PIN = 14;
constexpr unsigned I2C1_SCL_PIN = 15;
constexpr unsigned I2C0_SDA_PIN = 16;
constexpr unsigned I2C0_SCL_PIN = 17;

// Pressure sensor on I2C1.
constexpr uint8_t SDP610_I2C_ADDR     = 0x40;
constexpr int     SDP610_SCALE_FACTOR = 240;     // from the datasheet
constexpr float   SDP610_ALT_CORRECTION = 1.0f;  // set for the site altitude

// EEPROM chip.
constexpr uint8_t  EEPROM_I2C_ADDR  = 0x50;
constexpr uint16_t EEPROM_PAGE_SIZE = 64;
constexpr uint16_t EEPROM_SETTINGS_ADDR = 0;

// Control limits and valve timing.
constexpr uint16_t SETPOINT_MAX_PPM     = 1500;
constexpr uint16_t SETPOINT_DEFAULT_PPM = 800;
constexpr int16_t  CO2_SAFETY_PPM       = 2000;
constexpr uint32_t VALVE_OPEN_MS        = 1000;
constexpr uint32_t VALVE_HOLD_MS        = 30000;
constexpr uint32_t CONTROL_PERIOD_MS    = 1000;
constexpr uint32_t SENSOR_STALE_MS      = 10000;

// How often things run.
constexpr uint32_t SENSOR_POLL_MS  = 2000;
constexpr uint32_t UI_REFRESH_MS   = 500;
constexpr uint16_t SETPOINT_STEP_PPM = 10;

// Optional features.
constexpr bool ENABLE_NETWORK_TASK = true;
constexpr bool ENABLE_CONSOLE_TASK = true;
constexpr bool VERBOSE_LOG = false;      // true for extra debug prints

// Reporting and status-print intervals.
constexpr uint32_t THINGSPEAK_INTERVAL_MS = 20000;
constexpr uint32_t STATUS_PRINT_MS = 3000;

}
