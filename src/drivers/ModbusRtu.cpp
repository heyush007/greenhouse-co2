#include "ModbusRtu.h"
#include "FreeRTOS.h"
#include "task.h"

ModbusRtu::ModbusRtu(PicoOsUart &uart, int response_timeout_ms)
    : uart(uart), timeout_ms(response_timeout_ms) {}

uint16_t ModbusRtu::crc16(const uint8_t *data, int len) {
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
        }
    }
    return crc;
}

int ModbusRtu::transact(uint8_t *req, int len, uint8_t *resp, int expected_len) {
    const uint16_t crc = crc16(req, len);
    req[len]     = crc & 0xFF;        // CRC is sent low byte first
    req[len + 1] = crc >> 8;

    // Modbus RTU needs >= 3.5 character times of silence between frames (~4 ms at 9600).
    vTaskDelay(pdMS_TO_TICKS(5));
    uart.flush();                      // drop any stale bytes from a previous timeout
    uart.write(req, len + 2);

    // Read the fixed part first; an exception reply is only 5 bytes long.
    int got = uart.read(resp, 5, timeout_ms);
    if (got < 5) { ++errors; return -1; }

    if (resp[1] & 0x80) {              // exception: addr, fn|0x80, code, crc lo, crc hi
        ++errors;
        return -1;
    }
    if (expected_len > 5) {
        got += uart.read(resp + 5, expected_len - 5, timeout_ms);
    }
    if (got != expected_len || resp[0] != req[0] || resp[1] != req[1]) { ++errors; return -1; }

    const uint16_t rx_crc = resp[got - 2] | (resp[got - 1] << 8);
    if (rx_crc != crc16(resp, got - 2)) { ++errors; return -1; }

    ++ok;
    return got;
}

bool ModbusRtu::read_registers(uint8_t fn, uint8_t server, uint16_t reg, uint16_t count, uint16_t *out) {
    if (count == 0 || count > 16) return false;
    uint8_t req[8] = { server, fn,
                       static_cast<uint8_t>(reg >> 8),   static_cast<uint8_t>(reg & 0xFF),
                       static_cast<uint8_t>(count >> 8), static_cast<uint8_t>(count & 0xFF) };
    uint8_t resp[5 + 2 * 16];
    const int expected = 5 + 2 * count;  // addr, fn, byte count, data..., crc(2)
    if (transact(req, 6, resp, expected) < 0) return false;
    if (resp[2] != 2 * count) { ++errors; --ok; return false; }
    for (int i = 0; i < count; ++i) {
        out[i] = (resp[3 + 2 * i] << 8) | resp[4 + 2 * i];  // register data is big-endian
    }
    return true;
}

bool ModbusRtu::read_holding(uint8_t server, uint16_t reg, uint16_t count, uint16_t *out) {
    return read_registers(0x03, server, reg, count, out);
}

bool ModbusRtu::read_input(uint8_t server, uint16_t reg, uint16_t count, uint16_t *out) {
    return read_registers(0x04, server, reg, count, out);
}

bool ModbusRtu::write_single(uint8_t server, uint16_t reg, uint16_t value) {
    uint8_t req[8] = { server, 0x06,
                       static_cast<uint8_t>(reg >> 8),   static_cast<uint8_t>(reg & 0xFF),
                       static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF) };
    uint8_t resp[8];
    return transact(req, 6, resp, 8) == 8;  // the reply echoes the request
}
