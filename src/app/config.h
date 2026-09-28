// Every hardware constant and tuning value lives here, nowhere else.
// Values marked [VERIFY] come from documents, not from testing on the box.
#pragma once
#include <cstdint>

namespace cfg {

// ---------- Modbus RTU on UART1 (Pico add-on board schematic: Tx1 = GP4, Rx1 = GP5)
constexpr int MODBUS_UART      = 1;
constexpr int MODBUS_TX_PIN    = 4;
constexpr int MODBUS_RX_PIN    = 5;
constexpr int MODBUS_BAUD      = 9600;   // spec: all Modbus devices at 9600 bps
constexpr int MODBUS_STOP_BITS = 2;      // Vaisala default is 2. RP2040 only checks the first
                                         // stop bit on receive, so 2 also works with 1-stop devices.
constexpr int MODBUS_RESPONSE_TIMEOUT_MS = 100;

// Device addresses (spec)
constexpr uint8_t ADDR_MIO    = 1;
constexpr uint8_t ADDR_GMP252 = 240;
constexpr uint8_t ADDR_HMP60  = 241;

// Register ADDRESSES as sent on the wire (register number - 1)
constexpr uint16_t GMP252_CO2_PPM_INT16 = 0x0100; // reg 257, function 03, ppm
constexpr uint16_t HMP60_RH_X10         = 0x0100; // reg 257, function 03, %RH * 10
constexpr uint16_t HMP60_T_X10          = 0x0101; // reg 258, function 03, degC * 10
constexpr uint16_t MIO_AO1              = 0;      // holding 40001, function 06, 0..1000 = 0..100.0 %
constexpr uint16_t MIO_AI1_COUNTER      = 4;      // input 30005, function 04, self-clearing

// ---------- GPIO (Pico add-on board schematic)
constexpr unsigned VALVE_PIN   = 27;  // spec: CO2 valve relay
constexpr unsigned ROT_A_PIN   = 10;
constexpr unsigned ROT_B_PIN   = 11;
constexpr unsigned ROT_SW_PIN  = 12;  // active low, needs internal pull-up
constexpr unsigned SW0_PIN     = 9;   // active low
constexpr unsigned SW1_PIN     = 8;
constexpr unsigned SW2_PIN     = 7;

// ---------- I2C
constexpr unsigned I2C1_SDA_PIN = 14; // OLED + SDP610 (spec)
constexpr unsigned I2C1_SCL_PIN = 15;
constexpr unsigned I2C0_SDA_PIN = 16; // EEPROM
constexpr unsigned I2C0_SCL_PIN = 17;
constexpr uint8_t  EEPROM_I2C_ADDR  = 0x50;  // [VERIFY] typical 24Cxx address
constexpr uint16_t EEPROM_PAGE_SIZE = 64;    // [VERIFY] 24C256 page size
constexpr uint16_t EEPROM_SETTINGS_ADDR = 0;

// ---------- Control rules (spec)
constexpr uint16_t SETPOINT_MAX_PPM     = 1500;
constexpr uint16_t SETPOINT_DEFAULT_PPM = 800;
constexpr int16_t  CO2_SAFETY_PPM       = 2000;
constexpr uint32_t VALVE_OPEN_MS        = 1000;   // spec: about 1 s (never more than 2 s)
constexpr uint32_t VALVE_HOLD_MS        = 30000;  // spec: wait at least 30 s
constexpr uint32_t CONTROL_PERIOD_MS    = 1000;
constexpr uint32_t SENSOR_STALE_MS      = 10000;  // older data counts as missing -> valve closed

// ---------- Timing
constexpr uint32_t SENSOR_POLL_MS  = 2000;
constexpr uint32_t UI_REFRESH_MS   = 500;
constexpr uint16_t SETPOINT_STEP_PPM = 10;

// ---------- Optional features
constexpr bool ENABLE_NETWORK_TASK = false;  // flip when the group starts on ThingSpeak

} // namespace cfg
