#include "Eeprom.h"
#include "config.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"

namespace {
i2c_inst_t *const bus = i2c0;
constexpr uint32_t I2C_TIMEOUT_US = 20000;

void wait_ms(uint32_t ms) {
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) vTaskDelay(pdMS_TO_TICKS(ms));
    else busy_wait_ms(ms);
}

bool wait_write_cycle() {
    uint8_t dummy;
    for (int i = 0; i < 20; ++i) {
        if (i2c_read_timeout_us(bus, cfg::EEPROM_I2C_ADDR, &dummy, 1, false, I2C_TIMEOUT_US) >= 0) return true;
        wait_ms(1);
    }
    return false;
}
}

namespace eeprom {

void init() {
    i2c_init(bus, 100 * 1000);
    gpio_set_function(cfg::I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(cfg::I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(cfg::I2C0_SDA_PIN);
    gpio_pull_up(cfg::I2C0_SCL_PIN);
}

bool read(uint16_t addr, uint8_t *data, size_t len) {
    const uint8_t a[2] = { static_cast<uint8_t>(addr >> 8), static_cast<uint8_t>(addr & 0xFF) };
    if (i2c_write_timeout_us(bus, cfg::EEPROM_I2C_ADDR, a, 2, true, I2C_TIMEOUT_US) != 2) return false;
    return i2c_read_timeout_us(bus, cfg::EEPROM_I2C_ADDR, data, len, false,
                               I2C_TIMEOUT_US * (1 + len / 16)) == static_cast<int>(len);
}

bool write(uint16_t addr, const uint8_t *data, size_t len) {
    while (len > 0) {

        const size_t room  = cfg::EEPROM_PAGE_SIZE - (addr % cfg::EEPROM_PAGE_SIZE);
        const size_t chunk = len < room ? len : room;
        uint8_t buf[2 + cfg::EEPROM_PAGE_SIZE];
        buf[0] = addr >> 8;
        buf[1] = addr & 0xFF;
        for (size_t i = 0; i < chunk; ++i) buf[2 + i] = data[i];
        if (i2c_write_timeout_us(bus, cfg::EEPROM_I2C_ADDR, buf, 2 + chunk, false,
                                 I2C_TIMEOUT_US * 4) != static_cast<int>(2 + chunk)) return false;
        if (!wait_write_cycle()) return false;
        addr += chunk;
        data += chunk;
        len  -= chunk;
    }
    return true;
}

}
