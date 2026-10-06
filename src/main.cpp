// Startup: load settings, create the queues, then start the tasks.
#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "config.h"
#include "app_queues.h"
#include "settings.h"
#include "Eeprom.h"
#include "InputIsr.h"
#include "tasks.h"

// Used by FreeRTOS for run-time statistics.
extern "C" uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}

// Called if any task overflows its stack.
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void) xTask;
    printf("FATAL: stack overflow in task %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    while (true) {}
}
namespace {
// Task parameters must outlive main(), so they are static.
Settings boot_settings;
Settings network_snapshot;

// Create a task and stop everything if it fails.
void create_task(TaskFunction_t fn, const char *name, uint16_t stack_words, void *param, UBaseType_t prio) {
    if (xTaskCreate(fn, name, stack_words, param, tskIDLE_PRIORITY + prio, nullptr) != pdPASS) {
        printf("FATAL: could not create task %s (FreeRTOS heap too small?)\n", name);
        while (true) tight_loop_contents();
    }
}
}

int main() {
    stdio_init_all();
    printf("\n--- Greenhouse CO2 controller boot ---\n");

    // Read settings before the scheduler starts, so only Storage touches EEPROM later.
    eeprom::init();
    const bool loaded = settings_load(boot_settings);
    printf("Settings %s, setpoint %u ppm\n", loaded ? "loaded from EEPROM" : "not found, using defaults",
           boot_settings.setpoint_ppm);
    network_snapshot = boot_settings;

    if (!create_app_queues()) {
        printf("FATAL: queue allocation failed\n");
        while (true) tight_loop_contents();
    }

    // Hand the saved setpoint to Control through its normal queue.
    SetpointRequest sp{boot_settings.setpoint_ppm};
    xQueueSend(setpointQueue, &sp, 0);

    input_isr_init();

    // A higher priority number runs first.
    create_task(control_task, "control", 512,   nullptr,           4);
    create_task(modbus_task,  "modbus",  768,   nullptr,           3);
    create_task(ui_task,      "ui",      768,   &network_snapshot, 2);
    create_task(storage_task, "storage", 512,   &boot_settings,    1);
    if (cfg::ENABLE_NETWORK_TASK) {
        create_task(network_task, "network", 2048, &network_snapshot, 1);
    }
    if (cfg::ENABLE_CONSOLE_TASK) {
        create_task(console_task, "console", 1024, nullptr,           1);
    }

    vTaskStartScheduler();
    while (true) {}
}
