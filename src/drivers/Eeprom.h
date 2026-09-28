// 24Cxx-style I2C EEPROM on I2C0. Only the Storage task (and main() before the
// scheduler starts) may call this.
#pragma once
#include <cstdint>
#include <cstddef>

namespace eeprom {
void init();
bool read(uint16_t addr, uint8_t *data, size_t len);
bool write(uint16_t addr, const uint8_t *data, size_t len);  // splits on page boundaries
}
