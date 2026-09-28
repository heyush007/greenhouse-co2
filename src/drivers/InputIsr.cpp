#include "InputIsr.h"
#include "config.h"
#include "app_queues.h"
#include "shared_types.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

namespace {
constexpr uint32_t DEBOUNCE_US = 150 * 1000;
uint32_t last_press_us[32] = {};

void post(InputEvent ev, BaseType_t *woken) {
    // If the queue is full the event is dropped: the UI is behind anyway.
    xQueueSendToBackFromISR(inputQueue, &ev, woken);
}

// One callback per core in the Pico SDK, so every input pin shares it.
// Keep it short: decide what happened, post it, return. All logic lives in the UI task.
void gpio_callback(uint gpio, uint32_t events) {
    BaseType_t woken = pdFALSE;

    if (gpio == cfg::ROT_A_PIN && (events & GPIO_IRQ_EDGE_RISE)) {
        // Direction from B at the moment A rises. If the knob feels reversed, swap CW/CCW here.
        post(gpio_get(cfg::ROT_B_PIN) ? InputEvent::RotCCW : InputEvent::RotCW, &woken);
    } else if (events & GPIO_IRQ_EDGE_FALL) {
        const uint32_t now = time_us_32();
        if (now - last_press_us[gpio] >= DEBOUNCE_US) {
            last_press_us[gpio] = now;
            if      (gpio == cfg::ROT_SW_PIN) post(InputEvent::RotPress, &woken);
            else if (gpio == cfg::SW0_PIN)    post(InputEvent::Sw0, &woken);
            else if (gpio == cfg::SW1_PIN)    post(InputEvent::Sw1, &woken);
            else if (gpio == cfg::SW2_PIN)    post(InputEvent::Sw2, &woken);
        }
    }
    portYIELD_FROM_ISR(woken);
}

void setup_input(unsigned pin, bool pull_up) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    if (pull_up) gpio_pull_up(pin);
}
} // namespace

void input_isr_init() {
    setup_input(cfg::ROT_A_PIN, false);   // board has external pull-ups on A and B
    setup_input(cfg::ROT_B_PIN, false);
    setup_input(cfg::ROT_SW_PIN, true);
    setup_input(cfg::SW0_PIN, true);
    setup_input(cfg::SW1_PIN, true);
    setup_input(cfg::SW2_PIN, true);

    // The first call registers the shared callback; the rest only enable their pins.
    gpio_set_irq_enabled_with_callback(cfg::ROT_A_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled(cfg::ROT_SW_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(cfg::SW0_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(cfg::SW1_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(cfg::SW2_PIN, GPIO_IRQ_EDGE_FALL, true);
}
