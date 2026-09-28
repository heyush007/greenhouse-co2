// Control task: the only code that decides what the valve and fan do.
// Runs every CONTROL_PERIOD_MS. Never blocks for the 30 s hold; it counts ticks instead,
// so the 2000 ppm safety check still runs every second.
#include <cstdio>
#include "tasks.h"
#include "config.h"
#include "app_queues.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"

namespace {

bool elapsed(TickType_t since, uint32_t ms) {
    return (xTaskGetTickCount() - since) >= pdMS_TO_TICKS(ms);
}

void set_valve(bool open) {
    gpio_put(cfg::VALVE_PIN, open);
}

} // namespace

void control_task(void *) {
    gpio_init(cfg::VALVE_PIN);
    gpio_set_dir(cfg::VALVE_PIN, GPIO_OUT);
    set_valve(false);

    SystemStatus st{};
    st.setpoint_ppm = cfg::SETPOINT_DEFAULT_PPM;   // replaced by the saved value from main()

    bool       valve_open   = false;
    TickType_t valve_opened = 0;
    // Pretend the last injection finished long ago, so the first one is allowed once CO2 is known.
    TickType_t valve_closed = xTaskGetTickCount() - pdMS_TO_TICKS(cfg::VALVE_HOLD_MS);
    uint8_t    fan_sent     = 255;                 // forces the first fan command out

    TickType_t last_wake = xTaskGetTickCount();
    while (true) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(cfg::CONTROL_PERIOD_MS));

        // 1. New setpoints from UI / Network (non-blocking, take the latest)
        SetpointRequest req;
        while (xQueueReceive(setpointQueue, &req, 0) == pdTRUE) {
            st.setpoint_ppm = req.ppm > cfg::SETPOINT_MAX_PPM ? cfg::SETPOINT_MAX_PPM : req.ppm;
            printf("[control] setpoint %u ppm\n", st.setpoint_ppm);
        }

        // 2. Latest sensor data
        const bool have_data = xQueuePeek(sensorData, &st.sensors, 0) == pdTRUE;
        const bool co2_valid = have_data && st.sensors.co2_ok &&
                               !elapsed(st.sensors.timestamp, cfg::SENSOR_STALE_MS);
        const int16_t co2 = st.sensors.co2_ppm;

        // 3. Safety venting: above 2000 ppm run the fan until CO2 is back at the setpoint
        if (co2_valid && co2 > cfg::CO2_SAFETY_PPM) st.venting = true;
        if (co2_valid && st.venting && co2 <= st.setpoint_ppm) st.venting = false;
        st.fan_percent = st.venting ? 100 : 0;

        // 4. Valve: short pulse, then hold. Closed whenever data is missing or venting.
        if (valve_open && (elapsed(valve_opened, cfg::VALVE_OPEN_MS) || !co2_valid || st.venting)) {
            valve_open = false;
            valve_closed = xTaskGetTickCount();
        } else if (!valve_open && co2_valid && !st.venting && co2 < st.setpoint_ppm &&
                   elapsed(valve_closed, cfg::VALVE_HOLD_MS)) {
            valve_open = true;
            valve_opened = xTaskGetTickCount();
        }
        set_valve(valve_open);
        st.valve_open = valve_open;

        // 5. Fan command only on change; if the queue is full, retry next period
        if (st.fan_percent != fan_sent) {
            FanCommand cmd{st.fan_percent};
            if (xQueueSend(fanCmdQueue, &cmd, 0) == pdTRUE) fan_sent = st.fan_percent;
        }

        // 6. Publish for UI and Network
        xQueueOverwrite(systemStatus, &st);
    }
}
