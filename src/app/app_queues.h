// Kernel objects from the system diagram. Created once in main() before the scheduler starts.
#pragma once
#include "FreeRTOS.h"
#include "queue.h"

extern QueueHandle_t sensorData;     // mailbox, len 1: Modbus -> Control
extern QueueHandle_t fanCmdQueue;    // Control -> Modbus
extern QueueHandle_t systemStatus;   // mailbox, len 1: Control -> UI, Network
extern QueueHandle_t setpointQueue;  // UI, Network -> Control
extern QueueHandle_t saveQueue;      // UI, Network -> Storage
extern QueueHandle_t inputQueue;     // GPIO ISR -> UI

// Returns false if any queue could not be allocated (FreeRTOS heap too small).
bool create_app_queues();
