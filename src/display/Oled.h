// Minimal SSD1306 OLED driver; drawing comes from mono_vlsb.
#pragma once
#include "mono_vlsb.h"
#include "hardware/i2c.h"

class Oled : public mono_vlsb {
public:
    // Set up the I2C bus and pins; call once before creating an Oled.
    static void bus_init(unsigned sda_pin, unsigned scl_pin, uint32_t hz = 400000);

    explicit Oled(i2c_inst_t *i2c, uint8_t address = 0x3C, uint16_t width = 128, uint16_t height = 64);
    void show();   // push the frame buffer to the display

private:
    void send_cmd(uint8_t cmd);
    i2c_inst_t *i2c;
    uint8_t address;
};
