// The queues the tasks use to talk to each other.
#pragma once
#include "FreeRTOS.h"
#include "queue.h"

extern QueueHandle_t sensorData;     // Modbus -> Control (latest readings)
extern QueueHandle_t fanCmdQueue;    // Control -> Modbus (fan speed)
extern QueueHandle_t systemStatus;   // Control -> UI, Network (full status)
extern QueueHandle_t setpointQueue;  // UI, Network -> Control (new setpoint)
extern QueueHandle_t saveQueue;      // UI, Network -> Storage (save a setting)
extern QueueHandle_t inputQueue;     // input ISR -> UI (button/encoder events)
extern QueueHandle_t netStatus;      // Network -> UI (WiFi status)

// Create all the queues; returns false if memory ran out.
bool create_app_queues();
