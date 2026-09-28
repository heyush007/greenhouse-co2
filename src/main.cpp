// Greenhouse CO2 controller - startup.
// Order matters: load settings -> create queues -> hook ISRs -> create tasks -> start scheduler.
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

// The course template's FreeRTOSConfig.h uses this for run-time statistics.
// If the linker reports it as defined twice, delete this copy.
extern "C" uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
// FreeRTOSConfig.h turns on stack overflow checking, so the kernel needs this function.
// It runs when a task uses more stack than it was given in create_task() below.
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void) xTask;
    printf("FATAL: stack overflow in task %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    while (true) {}
}
namespace {
// Task parameters must outlive main(), hence static.
Settings boot_settings;
Settings network_snapshot;

void create_task(TaskFunction_t fn, const char *name, uint16_t stack_words, void *param, UBaseType_t prio) {
    if (xTaskCreate(fn, name, stack_words, param, tskIDLE_PRIORITY + prio, nullptr) != pdPASS) {
        printf("FATAL: could not create task %s (FreeRTOS heap too small?)\n", name);
        while (true) tight_loop_contents();
    }
}
} // namespace

int main() {
    stdio_init_all();                 // UART0 debug output
    printf("\n--- Greenhouse CO2 controller boot ---\n");

    // Settings are read before the scheduler starts, so the Storage task is the only
    // I2C0 user afterwards and no mutex is needed.
    eeprom::init();
    const bool loaded = settings_load(boot_settings);
    printf("Settings %s, setpoint %u ppm\n", loaded ? "loaded from EEPROM" : "not found, using defaults",
           boot_settings.setpoint_ppm);
    network_snapshot = boot_settings;

    if (!create_app_queues()) {
        printf("FATAL: queue allocation failed\n");
        while (true) tight_loop_contents();
    }

    // Hand the saved setpoint to the Control task through its normal input queue.
    SetpointRequest sp{boot_settings.setpoint_ppm};
    xQueueSend(setpointQueue, &sp, 0);

    input_isr_init();

    //          function      name       stack  param              priority (higher runs first)
    create_task(control_task, "control", 512,   nullptr,           4);
    create_task(modbus_task,  "modbus",  768,   nullptr,           3);
    create_task(ui_task,      "ui",      768,   nullptr,           2);
    create_task(storage_task, "storage", 512,   &boot_settings,    1);
    if (cfg::ENABLE_NETWORK_TASK) {
        create_task(network_task, "network", 1024, &network_snapshot, 1);
    }

    vTaskStartScheduler();
    while (true) {}                   // only reached if the scheduler could not start
}
