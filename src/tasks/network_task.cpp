// Network task: sends readings to ThingSpeak and gets setpoint commands back.
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "tasks.h"
#include "config.h"
#include "app_queues.h"
#include "IPStack.h"
#include "lwip/netif.h"
#include "FreeRTOS.h"
#include "task.h"

// Credentials and keys come from the build if EEPROM has none.
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif
#ifndef THINGSPEAK_HOST
#define THINGSPEAK_HOST ""
#endif
#ifndef THINGSPEAK_API_KEY
#define THINGSPEAK_API_KEY ""
#endif
#ifndef THINGSPEAK_TALKBACK_KEY
#define THINGSPEAK_TALKBACK_KEY ""
#endif
#ifndef THINGSPEAK_TALKBACK_ID
#define THINGSPEAK_TALKBACK_ID ""
#endif

namespace {

constexpr int HTTP_PORT = 80;

// The ThingSpeak server, defaulting to the public host.
const char *ts_host() {
    return (strlen(THINGSPEAK_HOST) > 0) ? THINGSPEAK_HOST : "api.thingspeak.com";
}

// Send one HTTP request and read the whole response into resp.
int http_exchange(IPStack &ip, const char *req, int req_len, char *resp, int resp_sz) {
    if (ip.connect(ts_host(), HTTP_PORT) != 0) {
        printf("[network] connect failed\n");
        return 0;
    }
    ip.write((unsigned char *) req, req_len, 2000);

    // Keep reading until the server stops sending.
    int total = 0;
    while (total < resp_sz - 1) {
        int n = ip.read((unsigned char *) resp + total, resp_sz - 1 - total, 1500);
        if (n <= 0) break;
        total += n;
    }
    resp[total] = '\0';
    ip.disconnect();
    return total;
}

// POST the five measurement fields to the channel.
void post_update(IPStack &ip, const SystemStatus &st, const char *api_key) {
    if (strlen(api_key) == 0) {
        printf("[network] no ThingSpeak API key - skipping update\n");
        return;
    }
    char body[192];
    int blen = snprintf(body, sizeof(body),
        "api_key=%s&field1=%d&field2=%.1f&field3=%.1f&field4=%u&field5=%u",
        api_key, st.sensors.co2_ppm, st.sensors.rh_pct, st.sensors.temp_c,
        st.fan_percent, st.setpoint_ppm);

    char req[352];
    int rlen = snprintf(req, sizeof(req),
        "POST /update.json HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s",
        ts_host(), blen, body);

    char resp[512];
    int n = http_exchange(ip, req, rlen, resp, sizeof(resp));
    printf("[network] update co2=%d rh=%.1f t=%.1f fan=%u sp=%u (resp %d bytes)\n",
           st.sensors.co2_ppm, st.sensors.rh_pct, st.sensors.temp_c,
           st.fan_percent, st.setpoint_ppm, n);
}

// Return the first number found in a string, or -1.
int first_int(const char *s) {
    while (*s && (*s < '0' || *s > '9')) ++s;
    return *s ? atoi(s) : -1;
}

// Ask TalkBack for the next command and apply it as a new setpoint.
void poll_talkback(IPStack &ip, const char *tb_key, const char *tb_id) {
    if (strlen(tb_key) == 0 || strlen(tb_id) == 0) return;

    char body[96];
    int blen = snprintf(body, sizeof(body), "api_key=%s", tb_key);
    char req[288];
    int rlen = snprintf(req, sizeof(req),
        "POST /talkbacks/%s/commands/execute.json HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s",
        tb_id, ts_host(), blen, body);

    // Static because 1.5 KB would overflow the task stack.
    static char resp[1536];
    int n = http_exchange(ip, req, rlen, resp, sizeof(resp));
    if (n <= 0) return;

    // Find the command value in the JSON reply.
    char *p = strstr(resp, "command_string");
    if (!p) {
        if (cfg::VERBOSE_LOG) printf("[network] talkback: no command (resp %d bytes)\n", n);
        return;
    }
    if (!(p = strchr(p, ':')) || !(p = strchr(p, '"'))) return;
    int v = first_int(p + 1);
    if (v > 0) {
        uint16_t ppm = v > cfg::SETPOINT_MAX_PPM ? cfg::SETPOINT_MAX_PPM : (uint16_t) v;
        SetpointRequest sp{ppm};
        xQueueSend(setpointQueue, &sp, pdMS_TO_TICKS(100));   // tell Control
        SaveRequest save{};
        save.field = SaveField::Setpoint;
        save.ppm = ppm;
        xQueueSend(saveQueue, &save, pdMS_TO_TICKS(100));     // tell Storage to persist
        printf("[network] talkback set setpoint -> %u ppm\n", ppm);
    }
}

// Publish the current link status and IP for the UI's WiFi screen.
void publish_net_status() {
    NetStatus ns{};
    ns.connected = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP;
    const ip4_addr_t *ip = netif_default ? netif_ip4_addr(netif_default) : nullptr;
    const char *s = (ip && ns.connected) ? ip4addr_ntoa(ip) : "-";
    for (size_t i = 0; i + 1 < sizeof(ns.ip) && s[i]; ++i) ns.ip[i] = s[i];
    xQueueOverwrite(netStatus, &ns);
}

}

void network_task(void *param) {
    const Settings creds = *static_cast<const Settings *>(param);

    // Prefer EEPROM values, fall back to the build-time ones.
    const char *ssid   = creds.wifi_ssid[0]              ? creds.wifi_ssid              : WIFI_SSID;
    const char *pw     = creds.wifi_password[0]          ? creds.wifi_password          : WIFI_PASSWORD;
    const char *ts_key = creds.thingspeak_api_key[0]     ? creds.thingspeak_api_key     : THINGSPEAK_API_KEY;
    const char *tb_key = creds.thingspeak_talkback_key[0]? creds.thingspeak_talkback_key: THINGSPEAK_TALKBACK_KEY;
    const char *tb_id  = creds.thingspeak_talkback_id[0] ? creds.thingspeak_talkback_id : THINGSPEAK_TALKBACK_ID;

    printf("[network] joining Wi-Fi SSID '%s'...\n", ssid);
    IPStack ipstack(ssid, pw);   // connects to Wi-Fi (can block up to 30 s)

    TickType_t wake = xTaskGetTickCount();
    while (true) {
        publish_net_status();
        SystemStatus st;
        if (xQueuePeek(systemStatus, &st, 0) == pdTRUE) {
            post_update(ipstack, st, ts_key);
            poll_talkback(ipstack, tb_key, tb_id);
        } else if (cfg::VERBOSE_LOG) {
            printf("[network] no system status yet\n");
        }
        xTaskDelayUntil(&wake, pdMS_TO_TICKS(cfg::THINGSPEAK_INTERVAL_MS));
    }
}
