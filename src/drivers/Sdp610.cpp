#include "Sdp610.h"
#include "config.h"

Sdp610::Sdp610(i2c_inst_t *i2c, uint8_t address) : i2c(i2c), address(address) {}

bool Sdp610::read(float *pa) {
    // Ask the sensor to measure, then read 3 bytes (value + checksum).
    uint8_t cmd = 0xF1;
    if (i2c_write_blocking(i2c, address, &cmd, 1, true) < 0) return false;

    uint8_t buf[3];
    if (i2c_read_blocking(i2c, address, buf, 3, false) < 0) return false;

    int16_t raw = static_cast<int16_t>((buf[0] << 8) | buf[1]);
    // Scale to pascals and apply the altitude correction.
    *pa = (raw / static_cast<float>(cfg::SDP610_SCALE_FACTOR)) * cfg::SDP610_ALT_CORRECTION;
    return true;
}
