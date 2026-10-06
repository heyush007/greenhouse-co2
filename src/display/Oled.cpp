#include "Oled.h"
#include "hardware/gpio.h"

// SSD1306 command bytes.
#define SSD1306_SET_MEM_MODE        0x20
#define SSD1306_SET_COL_ADDR        0x21
#define SSD1306_SET_PAGE_ADDR       0x22
#define SSD1306_SET_SCROLL          0x2E
#define SSD1306_SET_DISP_START_LINE 0x40
#define SSD1306_SET_CONTRAST        0x81
#define SSD1306_SET_CHARGE_PUMP     0x8D
#define SSD1306_SET_SEG_REMAP       0xA0
#define SSD1306_SET_ENTIRE_ON       0xA4
#define SSD1306_SET_NORM_DISP       0xA6
#define SSD1306_SET_MUX_RATIO       0xA8
#define SSD1306_SET_DISP            0xAE
#define SSD1306_SET_COM_OUT_DIR     0xC0
#define SSD1306_SET_DISP_OFFSET     0xD3
#define SSD1306_SET_DISP_CLK_DIV    0xD5
#define SSD1306_SET_PRECHARGE       0xD9
#define SSD1306_SET_COM_PIN_CFG     0xDA
#define SSD1306_SET_VCOM_DESEL      0xDB

// Set up I2C1 and its pins for the display.
void Oled::bus_init(unsigned sda_pin, unsigned scl_pin, uint32_t hz) {
    i2c_init(i2c1, hz);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

// Run the SSD1306 power-on command sequence.
Oled::Oled(i2c_inst_t *i2c_, uint8_t address_, uint16_t width, uint16_t height)
    : mono_vlsb(width, height, width, 1), i2c(i2c_), address(address_) {
    buffer.get()[0] = 0x40;
    uint8_t cmds[] = {
        SSD1306_SET_DISP,
        SSD1306_SET_MEM_MODE, 0x00,
        SSD1306_SET_DISP_START_LINE,
        SSD1306_SET_SEG_REMAP | 0x01,
        SSD1306_SET_MUX_RATIO, (uint8_t)(height - 1),
        SSD1306_SET_COM_OUT_DIR | 0x08,
        SSD1306_SET_DISP_OFFSET, 0x00,
        SSD1306_SET_COM_PIN_CFG, (uint8_t)(height > 32 ? 0x12 : 0x02),
        SSD1306_SET_DISP_CLK_DIV, 0x80,
        SSD1306_SET_PRECHARGE, 0xF1,
        SSD1306_SET_VCOM_DESEL, 0x30,
        SSD1306_SET_CONTRAST, 0xFF,
        SSD1306_SET_ENTIRE_ON,
        SSD1306_SET_NORM_DISP,
        SSD1306_SET_CHARGE_PUMP, 0x14,
        SSD1306_SET_SCROLL | 0x00,
        SSD1306_SET_DISP | 0x01,
    };
    for (uint8_t c : cmds) send_cmd(c);
}

void Oled::send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c, address, buf, 2, false);
}

void Oled::show() {
    send_cmd(SSD1306_SET_COL_ADDR);
    send_cmd(0);
    send_cmd(width - 1);
    send_cmd(SSD1306_SET_PAGE_ADDR);
    send_cmd(0);
    send_cmd(height / 8 - 1);
    buffer.get()[0] = 0x40;
    i2c_write_blocking(i2c, address, buffer.get(), size, false);
}
