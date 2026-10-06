#pragma once
#include <cstdint>
#include "PicoOsUart.h"

class ModbusRtu {
public:
    ModbusRtu(PicoOsUart &uart, int response_timeout_ms);
    bool read_holding(uint8_t server, uint16_t reg, uint16_t count, uint16_t *out);
    bool read_input(uint8_t server, uint16_t reg, uint16_t count, uint16_t *out);
    bool write_single(uint8_t server, uint16_t reg, uint16_t value);

    uint32_t ok_count() const { return ok; }
    uint32_t error_count() const { return errors; }

    static uint16_t crc16(const uint8_t *data, int len);

private:
    bool read_registers(uint8_t fn, uint8_t server, uint16_t reg, uint16_t count, uint16_t *out);

    int transact(uint8_t *req, int req_len_without_crc, uint8_t *resp, int expected_len);

    PicoOsUart &uart;
    int timeout_ms;
    uint32_t ok = 0;
    uint32_t errors = 0;
};
