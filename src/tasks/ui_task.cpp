// UI task: owns the OLED (I2C1) and all user input.
// Waits on inputQueue with a timeout, so it redraws at least every UI_REFRESH_MS.
//
// Controls:
//   Rotary press        start editing the setpoint / confirm and save
//   Rotate              change the setpoint while editing (SETPOINT_STEP_PPM per click)
//   SW0                 cancel editing
//
// TODO(UI owner): replace draw() with the OLED driver from the course template
// (SSD1306 on I2C1, SDA GP14 / SCL GP15). The printf version below lets the rest of the
// team work before the display code exists.
#include <cstdio>
#include "tasks.h"
#include "config.h"
#include "app_queues.h"
#include "FreeRTOS.h"
#include "task.h"

namespace {

struct UiState {
    bool     editing = false;
    uint16_t pending_ppm = 0;
};

void draw(const SystemStatus &st, bool have_status, const UiState &ui) {
    if (!have_status) {
        printf("[ui] waiting for data...\n");
        return;
    }
    const SensorData &s = st.sensors;
    printf("[ui] CO2 %4d ppm | RH %4.1f %% | T %4.1f C | fan %3u %%%s | valve %s | SP %s%u%s\n",
           s.co2_ppm, s.rh_pct, s.temp_c, st.fan_percent, s.fan_running ? "" : " (stopped)",
           st.valve_open ? "OPEN" : "shut",
           ui.editing ? "[" : "", ui.editing ? ui.pending_ppm : st.setpoint_ppm, ui.editing ? "]" : "");
}

void commit_setpoint(uint16_t ppm) {
    SetpointRequest sp{ppm};
    xQueueSend(setpointQueue, &sp, pdMS_TO_TICKS(100));

    SaveRequest save{};
    save.field = SaveField::Setpoint;
    save.ppm = ppm;
    xQueueSend(saveQueue, &save, pdMS_TO_TICKS(100));
}

} // namespace

void ui_task(void *) {
    UiState ui;
    SystemStatus st{};
    bool have_status = false;
    TickType_t last_draw = 0;

    while (true) {
        InputEvent ev;
        const bool got_event = xQueueReceive(inputQueue, &ev, pdMS_TO_TICKS(cfg::UI_REFRESH_MS)) == pdTRUE;
        have_status = xQueuePeek(systemStatus, &st, 0) == pdTRUE || have_status;

        if (got_event) {
            switch (ev) {
                case InputEvent::RotPress:
                    if (!ui.editing) {
                        ui.editing = true;
                        ui.pending_ppm = st.setpoint_ppm;
                    } else {
                        commit_setpoint(ui.pending_ppm);
                        ui.editing = false;
                    }
                    break;
                case InputEvent::RotCW:
                    if (ui.editing) {
                        const int v = ui.pending_ppm + cfg::SETPOINT_STEP_PPM;
                        ui.pending_ppm = v > cfg::SETPOINT_MAX_PPM ? cfg::SETPOINT_MAX_PPM : v;
                    }
                    break;
                case InputEvent::RotCCW:
                    if (ui.editing) {
                        ui.pending_ppm = ui.pending_ppm > cfg::SETPOINT_STEP_PPM
                                         ? ui.pending_ppm - cfg::SETPOINT_STEP_PPM : 0;
                    }
                    break;
                case InputEvent::Sw0:
                    ui.editing = false;
                    break;
                case InputEvent::Sw1:
                case InputEvent::Sw2:
                    break;   // free for the group to use (e.g. network settings menu)
            }
        }

        // Redraw on input immediately; otherwise at the refresh rate
        if (got_event || (xTaskGetTickCount() - last_draw) >= pdMS_TO_TICKS(cfg::UI_REFRESH_MS)) {
            draw(st, have_status, ui);
            last_draw = xTaskGetTickCount();
        }
    }
}
