// Control task: the only code that drives the valve and decides the fan speed.
#include <cstdio>
#include "tasks.h"
#include "config.h"
#include "app_queues.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"

namespace {

// True once ms have passed since the given tick.
bool elapsed(TickType_t since, uint32_t ms) {
    return (xTaskGetTickCount() - since) >= pdMS_TO_TICKS(ms);
}

// Open or close the CO2 valve.
void set_valve(bool open) {
    gpio_put(cfg::VALVE_PIN, open);
}

}

void control_task(void *){
    gpio_init(cfg::VALVE_PIN);
    gpio_set_dir(cfg::VALVE_PIN, GPIO_OUT);
    set_valve(false);

    SystemStatus st{};
    st.setpoint_ppm = cfg::SETPOINT_DEFAULT_PPM;

    bool       valve_open   = false;
    TickType_t valve_opened = 0;
    // Pretend the last injection was long ago so the first one is allowed.
    TickType_t valve_closed = xTaskGetTickCount() - pdMS_TO_TICKS(cfg::VALVE_HOLD_MS);
    uint8_t    fan_sent     = 255;
    TickType_t last_print   = 0;

    TickType_t last_wake = xTaskGetTickCount();
    while (true) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(cfg::CONTROL_PERIOD_MS));

        // Take any new setpoints.
        SetpointRequest req;
        while (xQueueReceive(setpointQueue, &req, 0) == pdTRUE) {
            st.setpoint_ppm = req.ppm > cfg::SETPOINT_MAX_PPM ? cfg::SETPOINT_MAX_PPM : req.ppm;
            printf("[control] setpoint %u ppm\n", st.setpoint_ppm);
        }

        // Get the latest readings and check they are fresh.
        const bool have_data = xQueuePeek(sensorData, &st.sensors, 0) == pdTRUE;
        const bool co2_valid = have_data && st.sensors.co2_ok &&
                               !elapsed(st.sensors.timestamp, cfg::SENSOR_STALE_MS);
        const int16_t co2 = st.sensors.co2_ppm;

        // Vent above the safety limit until CO2 is back to the setpoint.
        if (co2_valid && co2 > cfg::CO2_SAFETY_PPM) st.venting = true;
        if (co2_valid && st.venting && co2 <= st.setpoint_ppm) st.venting = false;
        st.fan_percent = st.venting ? 100 : 0;

        // Pulse the valve open when CO2 is below the setpoint.
        if (valve_open && (elapsed(valve_opened, cfg::VALVE_OPEN_MS) || !co2_valid || st.venting)) {
            valve_open = false;
            valve_closed = xTaskGetTickCount();
            printf("[control] valve CLOSED\n");
        } else if (!valve_open && co2_valid && !st.venting && co2 < st.setpoint_ppm &&
                   elapsed(valve_closed, cfg::VALVE_HOLD_MS)) {
            valve_open = true;
            valve_opened = xTaskGetTickCount();
            printf("[control] valve OPEN (co2=%d < setpoint=%u)\n", co2, st.setpoint_ppm);
        }
        set_valve(valve_open);
        st.valve_open = valve_open;

        // Send the fan speed only when it changes.
        if (st.fan_percent != fan_sent) {
            FanCommand cmd{st.fan_percent};
            if (xQueueSend(fanCmdQueue, &cmd, 0) == pdTRUE) fan_sent = st.fan_percent;
        }

        // Share the full status with the UI and network.
        xQueueOverwrite(systemStatus, &st);

        // Print a short status line now and then.
        if (elapsed(last_print, cfg::STATUS_PRINT_MS)) {
            last_print = xTaskGetTickCount();
            printf("[status] co2=%d sp=%u fan=%u%% %s%s\n",
                   co2, st.setpoint_ppm, st.fan_percent,
                   st.venting ? "venting " : "", st.valve_open ? "injecting" : "");
        }
    }
}
