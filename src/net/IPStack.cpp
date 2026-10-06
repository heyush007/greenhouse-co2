#include <cstring>
#include "pico/time.h"
#include "lwip/dns.h"
#include "FreeRTOS.h"
#include "task.h"

#include "IPStack.h"
#include "config.h"

#define DEBUG_printf printf
#define DUMP_BYTES(A, B) {}

#define TRACE_printf(...) do { if (cfg::VERBOSE_LOG) printf(__VA_ARGS__); } while (0)

IPStack::IPStack(const char *ssid, const char *pw) : tcp_pcb{nullptr}, dropped{0}, count{0}, wr{0}, rd{0}, connected{false} {
    if (cyw43_arch_init()) {
        DEBUG_printf("failed to initialise\n");
        return;
    }
    cyw43_arch_enable_sta_mode();

    DEBUG_printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pw, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        DEBUG_printf("Failed to connect.\n");
    } else {
        DEBUG_printf("Connected.\n");
    }

}

int IPStack::connect(uint32_t hostname, int port) {
    return ERR_ARG;
}

void IPStack::dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *arg) {
    auto self = static_cast<IPStack *>(arg);
    if (ipaddr) { self->remote_addr = *ipaddr; self->dns_ok = true; }
    self->dns_complete = true;
}

int IPStack::connect(const char *hostname, int port) {

    if (!ip4addr_aton(hostname, &remote_addr)) {
        dns_complete = false;
        dns_ok = false;
        cyw43_arch_lwip_begin();
        err_t e = dns_gethostbyname(hostname, &remote_addr, dns_found_cb, this);
        cyw43_arch_lwip_end();
        if (e == ERR_OK) {
            dns_ok = true;
        } else if (e == ERR_INPROGRESS) {
            absolute_time_t to = make_timeout_time_ms(10000);
            while (!dns_complete && !time_reached(to)) vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            DEBUG_printf("dns_gethostbyname failed %d\n", e);
            return ERR_ARG;
        }
        if (!dns_ok) {
            DEBUG_printf("DNS resolution failed for %s\n", hostname);
            return ERR_ARG;
        }
        TRACE_printf("Resolved %s -> %s\n", hostname, ip4addr_ntoa(&remote_addr));
    }

    TRACE_printf("Connecting to %s port %u\n", ip4addr_ntoa(&remote_addr), port);
    tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(remote_addr));
    if (!tcp_pcb) {
        DEBUG_printf("failed to create pcb\n");
        return ERR_MEM;
    }

    tcp_arg(tcp_pcb, this);
    tcp_poll(tcp_pcb, IPStack::tcp_client_poll, POLL_TIME_S * 2);
    tcp_sent(tcp_pcb, IPStack::tcp_client_sent);
    tcp_recv(tcp_pcb, IPStack::tcp_client_recv);
    tcp_err(tcp_pcb, IPStack::tcp_client_err);

    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(tcp_pcb, &remote_addr, port, IPStack::tcp_client_connected);
    cyw43_arch_lwip_end();

    return err;
}

err_t IPStack::tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {

    TRACE_printf("tcp_client_sent %u\n", len);

    return ERR_OK;
}

err_t IPStack::tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    auto state = static_cast<IPStack *>(arg);
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
    }
    state->connected = true;

    return ERR_OK;
}

err_t IPStack::tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {

    TRACE_printf("tcp_client_poll\n");
    return ERR_OK;
}

void IPStack::tcp_client_err(void *arg, err_t err) {

    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err %d\n", err);

    }
}

err_t IPStack::tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    auto state = static_cast<IPStack *>(arg);
    if (!p) {

        return ERR_OK;
    }

    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
#if 0
        DEBUG_printf("recv %d err %d\n", p->tot_len, err);
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            DUMP_BYTES(q->payload, q->len);
        }
#endif

        uint16_t available = BUF_SIZE - state->count;
        uint16_t bytes_to_copy = available > p->tot_len ? p->tot_len : available;
        uint16_t wr_end = state->wr + bytes_to_copy;
        uint16_t first_copy = 0;

        if (bytes_to_copy < p->tot_len) {
            state->dropped += p->tot_len - bytes_to_copy;
        }

        if (wr_end > BUF_SIZE) {

            first_copy = BUF_SIZE - state->wr;
            if (first_copy) {
                bytes_to_copy -= pbuf_copy_partial(p, state->buffer + state->wr, first_copy, 0);
                state->wr = 0;
            }
            state->count += first_copy;

        }
        state->wr += pbuf_copy_partial(p, state->buffer + state->wr, bytes_to_copy, first_copy);
        state->wr %= BUF_SIZE;
        state->count += bytes_to_copy;

        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    return ERR_OK;
}

int IPStack::read(unsigned char *buffer, int len, int timeout) {

    auto to = make_timeout_time_ms(timeout);
    do {
        cyw43_arch_poll();
    } while (count < len && !time_reached(to));

    uint16_t first_copy = 0;
    int bytes_to_copy = count < len ? count : len;
    if (bytes_to_copy) {
        uint16_t rd_end = rd + bytes_to_copy;
        if (rd_end > BUF_SIZE) {

            first_copy = BUF_SIZE - rd;
            if (first_copy) {
                std::memcpy(buffer, this->buffer + rd, first_copy);
                bytes_to_copy -= first_copy;
                count -= first_copy;
            }

            std::memcpy(buffer + first_copy, this->buffer, bytes_to_copy);
            rd = bytes_to_copy;

        } else {
            std::memcpy(buffer, this->buffer + rd, bytes_to_copy);
            rd = (rd + bytes_to_copy) % BUF_SIZE;
        }
        count -= bytes_to_copy;
    }

    return bytes_to_copy+first_copy;
}

int IPStack::write(unsigned char *buffer, int len, int timeout) {
    int rv = len;

    cyw43_arch_lwip_begin();

    err_t err = tcp_write(tcp_pcb, buffer, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        DEBUG_printf("Failed to write data %d\n", err);
        rv = -1;
    }

    if (tcp_output(tcp_pcb) != ERR_OK) {

        rv = -2;
    }

    cyw43_arch_lwip_end();

    return rv;
}

int IPStack::disconnect() {
    cyw43_arch_lwip_begin();

    err_t err = ERR_OK;
    if (tcp_pcb != nullptr) {
        tcp_arg(tcp_pcb, NULL);
        tcp_poll(tcp_pcb, NULL, 0);
        tcp_sent(tcp_pcb, NULL);
        tcp_recv(tcp_pcb, NULL);
        tcp_err(tcp_pcb, NULL);
        err = tcp_close(tcp_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(tcp_pcb);
            err = ERR_ABRT;
        }
        tcp_pcb = nullptr;
    }
    cyw43_arch_lwip_end();
    return err;
}
