#include "app_queues.h"
#include "shared_types.h"

QueueHandle_t sensorData    = nullptr;
QueueHandle_t fanCmdQueue   = nullptr;
QueueHandle_t systemStatus  = nullptr;
QueueHandle_t setpointQueue = nullptr;
QueueHandle_t saveQueue     = nullptr;
QueueHandle_t inputQueue    = nullptr;

bool create_app_queues() {
    sensorData    = xQueueCreate(1,  sizeof(SensorData));      // mailbox: xQueueOverwrite / xQueuePeek
    systemStatus  = xQueueCreate(1,  sizeof(SystemStatus));    // mailbox: xQueueOverwrite / xQueuePeek
    fanCmdQueue   = xQueueCreate(5,  sizeof(FanCommand));
    setpointQueue = xQueueCreate(5,  sizeof(SetpointRequest));
    saveQueue     = xQueueCreate(5,  sizeof(SaveRequest));
    inputQueue    = xQueueCreate(16, sizeof(InputEvent));

    vQueueAddToRegistry(sensorData,    "sensorData");          // names show up in the CLion RTOS view
    vQueueAddToRegistry(systemStatus,  "systemStatus");
    vQueueAddToRegistry(fanCmdQueue,   "fanCmdQueue");
    vQueueAddToRegistry(setpointQueue, "setpointQueue");
    vQueueAddToRegistry(saveQueue,     "saveQueue");
    vQueueAddToRegistry(inputQueue,    "inputQueue");

    return sensorData && systemStatus && fanCmdQueue && setpointQueue && saveQueue && inputQueue;
}
