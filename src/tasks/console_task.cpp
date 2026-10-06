// Console task: a simple command line over the debug UART.
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "pico/stdlib.h"
#include "tasks.h"
#include "config.h"
#include "app_queues.h"
#include "FreeRTOS.h"
#include "task.h"

namespace {

constexpr int LINE_MAX = 80;

// Print the list of commands.
void print_help() {
    printf("\nCommands:\n"
           "  help            this help\n"
           "  status          show measurements and setpoint\n"
           "  set <ppm>       set CO2 setpoint (10..%u)\n"
           "  ssid <text>     set Wi-Fi SSID\n"
           "  pass <text>     set Wi-Fi password\n"
           "  tskey <text>    set ThingSpeak write API key\n"
           "  tbkey <text>    set ThingSpeak TalkBack key\n"
           "  tbid <text>     set ThingSpeak TalkBack ID\n"
           "  reset           restore factory settings\n",
           cfg::SETPOINT_MAX_PPM);
}

// Print the latest readings.
void print_status() {
    SystemStatus st;
    if (xQueuePeek(systemStatus, &st, 0) != pdTRUE) {
        printf("no data yet\n");
        return;
    }
    const SensorData &s = st.sensors;
    printf("CO2=%d ppm  RH=%.1f %%  T=%.1f C  fan=%u%%  setpoint=%u ppm  %s%s\n",
           s.co2_ppm, s.rh_pct, s.temp_c, st.fan_percent, st.setpoint_ppm,
           st.venting ? "[venting] " : "", st.valve_open ? "[injecting]" : "");
}

// Send a text setting to the storage task.
void save_text(SaveField field, const char *text) {
    SaveRequest r{};
    r.field = field;
    size_t i = 0;
    for (; i + 1 < sizeof(r.text) && text[i]; ++i) r.text[i] = text[i];
    r.text[i] = '\0';
    xQueueSend(saveQueue, &r, pdMS_TO_TICKS(100));
}

// Parse and run one typed command line.
void handle_line(char *line) {
    // Split the line into a command and its argument.
    char *cmd = line;
    while (*cmd == ' ') ++cmd;
    char *arg = cmd;
    while (*arg && *arg != ' ') ++arg;
    if (*arg == ' ') { *arg = '\0'; ++arg; while (*arg == ' ') ++arg; }

    if (cmd[0] == '\0') return;

    if (strcmp(cmd, "help") == 0) {
        print_help();
    } else if (strcmp(cmd, "status") == 0) {
        print_status();
    } else if (strcmp(cmd, "set") == 0) {
        int v = atoi(arg);
        if (v < 10 || v > cfg::SETPOINT_MAX_PPM) {
            printf("setpoint must be 10..%u\n", cfg::SETPOINT_MAX_PPM);
            return;
        }
        SetpointRequest sp{(uint16_t) v};
        xQueueSend(setpointQueue, &sp, pdMS_TO_TICKS(100));   // tell Control
        SaveRequest save{};
        save.field = SaveField::Setpoint;
        save.ppm = (uint16_t) v;
        xQueueSend(saveQueue, &save, pdMS_TO_TICKS(100));      // tell Storage
        printf("setpoint -> %d ppm\n", v);
    } else if (strcmp(cmd, "ssid") == 0) {
        if (!arg[0]) { printf("usage: ssid <text>\n"); return; }
        save_text(SaveField::WifiSsid, arg);
        printf("SSID saved (takes effect on next boot)\n");
    } else if (strcmp(cmd, "pass") == 0) {
        if (!arg[0]) { printf("usage: pass <text>\n"); return; }
        save_text(SaveField::WifiPassword, arg);
        printf("Wi-Fi password saved (takes effect on next boot)\n");
    } else if (strcmp(cmd, "tskey") == 0) {
        if (!arg[0]) { printf("usage: tskey <text>\n"); return; }
        save_text(SaveField::ThingSpeakKey, arg);
        printf("ThingSpeak key saved (takes effect on next boot)\n");
    } else if (strcmp(cmd, "tbkey") == 0) {
        if (!arg[0]) { printf("usage: tbkey <text>\n"); return; }
        save_text(SaveField::TalkbackKey, arg);
        printf("TalkBack key saved (takes effect on next boot)\n");
    } else if (strcmp(cmd, "tbid") == 0) {
        if (!arg[0]) { printf("usage: tbid <text>\n"); return; }
        save_text(SaveField::TalkbackId, arg);
        printf("TalkBack ID saved (takes effect on next boot)\n");
    } else if (strcmp(cmd, "reset") == 0) {
        SaveRequest r{};
        r.field = SaveField::FactoryReset;
        xQueueSend(saveQueue, &r, pdMS_TO_TICKS(100));
        printf("factory settings restored (reboot to apply network settings)\n");
    } else {
        printf("unknown command '%s' - type help\n", cmd);
    }
}

}

void console_task(void *) {
    char line[LINE_MAX];
    int len = 0;

    printf("\n[console] ready\n");
    print_help();                 // show the command list at startup
    printf("> ");

    while (true) {
        // Read one character without blocking the scheduler.
        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (c == '\r' || c == '\n') {
            putchar('\n');
            line[len] = '\0';
            handle_line(line);
            len = 0;
            printf("> ");
        } else if (c == '\b' || c == 0x7f) {     // backspace
            if (len > 0) { --len; printf("\b \b"); }
        } else if (len < LINE_MAX - 1 && c >= ' ' && c < 0x7f) {
            line[len++] = (char) c;
            putchar(c);                          // echo what was typed
        }
    }
}
