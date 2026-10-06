// Modbus task: the only code on the Modbus bus; reads the sensors and sets the fan.
#include <cstdio>
#include "tasks.h"
#include "config.h"
#include "app_queues.h"
#include "PicoOsUart.h"
#include "ModbusRtu.h"
#include "FreeRTOS.h"
#include "task.h"

namespace {

// Read CO2, humidity/temperature and the fan pulse counter.
void poll_sensors(ModbusRtu &mb, SensorData &d, int &zero_reads) {
    uint16_t reg[2];

    d.co2_ok = mb.read_holding(cfg::ADDR_GMP252, cfg::GMP252_CO2_PPM_INT16, 1, reg);
    if (d.co2_ok) d.co2_ppm = static_cast<int16_t>(reg[0]);

    d.rh_t_ok = mb.read_holding(cfg::ADDR_HMP60, cfg::HMP60_RH_X10, 2, reg);
    if (d.rh_t_ok) {
        d.rh_pct = static_cast<int16_t>(reg[0]) / 10.0f;
        d.temp_c = static_cast<int16_t>(reg[1]) / 10.0f;
    }

    d.mio_ok = mb.read_input(cfg::ADDR_MIO, cfg::MIO_AI1_COUNTER, 1, reg);
    if (d.mio_ok) {
        d.fan_pulses = reg[0];
        // The counter clears on read, so two zero reads mean the fan has stopped.
        zero_reads = (reg[0] == 0) ? zero_reads + 1 : 0;
        d.fan_running = zero_reads < 2;
    }

    d.timestamp = xTaskGetTickCount();
}

}

void modbus_task(void *) {
    PicoOsUart uart(cfg::MODBUS_UART, cfg::MODBUS_TX_PIN, cfg::MODBUS_RX_PIN,
                    cfg::MODBUS_BAUD, cfg::MODBUS_STOP_BITS);
    ModbusRtu mb(uart, cfg::MODBUS_RESPONSE_TIMEOUT_MS);

    SensorData data{};
    int zero_reads = 0;
    uint8_t fan_percent = 0;
    bool fan_pending = true;

    const TickType_t poll_period = pdMS_TO_TICKS(cfg::SENSOR_POLL_MS);
    TickType_t next_poll = xTaskGetTickCount();

    while (true) {
        // Wait for a fan command, but wake up in time for the next poll.
        const TickType_t now = xTaskGetTickCount();
        const TickType_t wait = (static_cast<int32_t>(next_poll - now) > 0) ? next_poll - now : 0;
        FanCommand cmd;
        if (xQueueReceive(fanCmdQueue, &cmd, wait) == pdTRUE) {
            fan_percent = cmd.percent > 100 ? 100 : cmd.percent;
            fan_pending = true;
        }

        // Write the fan speed, retrying next loop if it fails.
        if (fan_pending) {
            fan_pending = !mb.write_single(cfg::ADDR_MIO, cfg::MIO_AO1, fan_percent * 10);
            if (fan_pending) printf("[modbus] fan write failed, retrying\n");
        }

        // Poll the sensors when due and publish the readings.
        if (static_cast<int32_t>(xTaskGetTickCount() - next_poll) >= 0) {
            poll_sensors(mb, data, zero_reads);
            xQueueOverwrite(sensorData, &data);
            next_poll += poll_period;
            if (cfg::VERBOSE_LOG)
                printf("[modbus] co2=%d%s rh=%.1f t=%.1f%s fan_pulses=%u%s\n",
                       data.co2_ppm, data.co2_ok ? "" : "(ERR)", data.rh_pct, data.temp_c,
                       data.rh_t_ok ? "" : "(ERR)", data.fan_pulses, data.mio_ok ? "" : "(ERR)");
        }
    }
}
