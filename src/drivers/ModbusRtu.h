// Minimal Modbus RTU master over PicoOsUart.
// Covers the three function codes this project needs:
//   03 Read Holding Registers  (GMP252, HMP60)
//   04 Read Input Registers    (MIO pulse counter)
//   06 Write Single Register   (MIO AO1 fan speed)
// Not thread-safe by design: only the Modbus task may own an instance.
// If your course template ships its own Modbus library, you can swap it in here:
// the Modbus task only calls these three methods.
#pragma once
#include <cstdint>
#include "PicoOsUart.h"

class ModbusRtu {
public:
    ModbusRtu(PicoOsUart &uart, int response_timeout_ms);
    bool read_holding(uint8_t server, uint16_t reg, uint16_t count, uint16_t *out);
    bool read_input(uint8_t server, uint16_t reg, uint16_t count, uint16_t *out);
    bool write_single(uint8_t server, uint16_t reg, uint16_t value);

    // Diagnostics for the debug console
    uint32_t ok_count() const { return ok; }
    uint32_t error_count() const { return errors; }

    static uint16_t crc16(const uint8_t *data, int len);

private:
    bool read_registers(uint8_t fn, uint8_t server, uint16_t reg, uint16_t count, uint16_t *out);
    // Sends req (CRC appended here) and receives the response into resp.
    // Returns the number of valid response bytes, or -1 on timeout/CRC/exception.
    int transact(uint8_t *req, int req_len_without_crc, uint8_t *resp, int expected_len);

    PicoOsUart &uart;
    int timeout_ms;
    uint32_t ok = 0;
    uint32_t errors = 0;
};
