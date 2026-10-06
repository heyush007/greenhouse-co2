#pragma once
#include <cstdint>
#include <cstddef>

namespace eeprom {
void init();
bool read(uint16_t addr, uint8_t *data, size_t len);
bool write(uint16_t addr, const uint8_t *data, size_t len);
}
