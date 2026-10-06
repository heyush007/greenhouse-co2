// UI task: owns the OLED and handles the rotary encoder and buttons.
#include <cstdio>
#include "tasks.h"
#include "config.h"
#include "app_queues.h"
#include "Oled.h"
#include "Sdp610.h"
#include "FreeRTOS.h"
#include "task.h"

// Build-time Wi-Fi credentials, used for the WiFi screen if EEPROM has none.
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

namespace {

// The three screens the UI can show.
enum class Screen { Menu, Monitor, Wifi };

// The menu options, in order.
const char *const MENU_ITEMS[] = { "Monitor", "WiFi" };
constexpr int MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

// Everything the UI needs to remember between redraws.
struct UiState {
    Screen   screen = Screen::Menu;
    int      sel = 0;              // which menu item is highlighted
    bool     editing = false;      // true while changing the setpoint
    uint16_t pending_ppm = 0;      // the setpoint being edited
};

// Draw the home menu with the current item highlighted.
void draw_menu(Oled &oled, const UiState &ui) {
    oled.fill(0);
    oled.text("Menu", 0, 0);
    for (int i = 0; i < MENU_COUNT; ++i) {
        int y = 20 + i * 12;
        if (i == ui.sel) {
            oled.rect(0, y - 1, 128, 10, 1, true);
            oled.text(MENU_ITEMS[i], 2, y, 0);
        } else {
            oled.text(MENU_ITEMS[i], 2, y);
        }
    }
    oled.text("push=open", 0, 56);
    oled.show();
}

// Draw the live readings screen.
void draw_monitor(Oled &oled, const SystemStatus &st, bool have_status, const UiState &ui,
                  float pressure_pa, bool pressure_ok) {
    char buf[24];
    oled.fill(0);
    oled.text("Greenhouse CO2", 0, 0);

    if (!have_status) {
        oled.text("waiting for data", 0, 24);
        oled.show();
        return;
    }
    const SensorData &s = st.sensors;

    snprintf(buf, sizeof(buf), "CO2: %4d ppm", s.co2_ppm);
    oled.text(buf, 0, 8);

    // Show the value being edited, highlighted, otherwise the saved setpoint.
    uint16_t sp = ui.editing ? ui.pending_ppm : st.setpoint_ppm;
    snprintf(buf, sizeof(buf), "Set: %4u ppm", sp);
    if (ui.editing) {
        oled.rect(0, 16, 128, 8, 1, true);
        oled.text(buf, 0, 16, 0);
    } else {
        oled.text(buf, 0, 16);
    }

    snprintf(buf, sizeof(buf), "RH:  %4.1f %%", s.rh_pct);
    oled.text(buf, 0, 24);
    snprintf(buf, sizeof(buf), "T:   %4.1f C", s.temp_c);
    oled.text(buf, 0, 32);
    snprintf(buf, sizeof(buf), "Fan: %3u %% %s", st.fan_percent, s.fan_running ? "RUN" : "OFF");
    oled.text(buf, 0, 40);
    if (pressure_ok) snprintf(buf, sizeof(buf), "dP:  %5.1f Pa", pressure_pa);
    else             snprintf(buf, sizeof(buf), "dP:     --");
    oled.text(buf, 0, 48);

    // Bottom line shows whatever the system is doing right now.
    if (!s.co2_ok || !s.rh_t_ok)  oled.text("** SENSOR ERROR **", 0, 56);
    else if (st.venting)          oled.text("venting (CO2 high)", 0, 56);
    else if (ui.editing)          oled.text("turn=set push=save", 0, 56);
    else if (st.valve_open)       oled.text("injecting CO2...", 0, 56);
    else                          oled.text("SW0=menu push=set", 0, 56);

    oled.show();
}

// Draw the WiFi info screen with the stored credentials and link status.
void draw_wifi(Oled &oled, const char *ssid, const char *pw) {
    char buf[24];
    oled.fill(0);
    oled.text("WiFi", 0, 0);

    snprintf(buf, sizeof(buf), "S:%s", ssid);
    oled.text(buf, 0, 14);
    snprintf(buf, sizeof(buf), "P:%s", pw);
    oled.text(buf, 0, 24);

    // Read the live status the network task publishes.
    NetStatus ns;
    if (xQueuePeek(netStatus, &ns, 0) != pdTRUE) {
        oled.text("net: off", 0, 38);
    } else if (ns.connected) {
        snprintf(buf, sizeof(buf), "up %s", ns.ip);
        oled.text(buf, 0, 38);
    } else {
        oled.text("connecting...", 0, 38);
    }

    oled.text("SW0=back", 0, 56);
    oled.show();
}

// Send a new setpoint to the control task and the storage task.
void commit_setpoint(uint16_t ppm) {
    SetpointRequest sp{ppm};
    xQueueSend(setpointQueue, &sp, pdMS_TO_TICKS(100));

    SaveRequest save{};
    save.field = SaveField::Setpoint;
    save.ppm = ppm;
    xQueueSend(saveQueue, &save, pdMS_TO_TICKS(100));
}

// React to one button or encoder event based on the current screen.
void handle_event(InputEvent ev, UiState &ui, const SystemStatus &st) {
    switch (ui.screen) {
        case Screen::Menu:
            switch (ev) {
                case InputEvent::RotCW:   ui.sel = (ui.sel + 1) % MENU_COUNT; break;
                case InputEvent::RotCCW:  ui.sel = (ui.sel + MENU_COUNT - 1) % MENU_COUNT; break;
                case InputEvent::RotPress:
                    ui.screen = (ui.sel == 0) ? Screen::Monitor : Screen::Wifi;
                    break;
                default: break;
            }
            break;

        case Screen::Monitor:
            switch (ev) {
                case InputEvent::RotPress:
                    // First press starts editing, second press saves.
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
                    // Cancel an edit, or go back to the menu.
                    if (ui.editing) ui.editing = false;
                    else            ui.screen = Screen::Menu;
                    break;
                default: break;
            }
            break;

        case Screen::Wifi:
            if (ev == InputEvent::Sw0) ui.screen = Screen::Menu;
            break;
    }
}

}

void ui_task(void *param) {
    const Settings *creds = static_cast<const Settings *>(param);
    // Use EEPROM credentials if set, otherwise the build-time ones.
    const char *ssid = (creds && creds->wifi_ssid[0])     ? creds->wifi_ssid     : WIFI_SSID;
    const char *pw   = (creds && creds->wifi_password[0]) ? creds->wifi_password : WIFI_PASSWORD;

    Oled::bus_init(cfg::I2C1_SDA_PIN, cfg::I2C1_SCL_PIN);
    Oled oled(i2c1, 0x3C);
    Sdp610 pressure(i2c1, cfg::SDP610_I2C_ADDR);   // shares the OLED's bus

    UiState ui;
    SystemStatus st{};
    bool have_status = false;
    float pressure_pa = 0.0f;
    bool pressure_ok = false;
    TickType_t last_draw = 0;

    while (true) {
        // Wait for an input, but wake up regularly to refresh the screen.
        InputEvent ev;
        const bool got_event = xQueueReceive(inputQueue, &ev, pdMS_TO_TICKS(cfg::UI_REFRESH_MS)) == pdTRUE;
        have_status = (xQueuePeek(systemStatus, &st, 0) == pdTRUE) || have_status;

        if (got_event) handle_event(ev, ui, st);

        if (got_event || (xTaskGetTickCount() - last_draw) >= pdMS_TO_TICKS(cfg::UI_REFRESH_MS)) {
            switch (ui.screen) {
                case Screen::Menu:    draw_menu(oled, ui); break;
                case Screen::Monitor:
                    pressure_ok = pressure.read(&pressure_pa);   // only read when shown
                    draw_monitor(oled, st, have_status, ui, pressure_pa, pressure_ok);
                    break;
                case Screen::Wifi:    draw_wifi(oled, ssid, pw); break;
            }
            last_draw = xTaskGetTickCount();
        }
    }
}
