#include "app_queues.h"
#include "shared_types.h"

QueueHandle_t sensorData    = nullptr;
QueueHandle_t fanCmdQueue   = nullptr;
QueueHandle_t systemStatus  = nullptr;
QueueHandle_t setpointQueue = nullptr;
QueueHandle_t saveQueue     = nullptr;
QueueHandle_t inputQueue    = nullptr;
QueueHandle_t netStatus     = nullptr;

bool create_app_queues() {
    // Length-1 queues act as mailboxes that hold only the newest value.
    sensorData    = xQueueCreate(1,  sizeof(SensorData));
    systemStatus  = xQueueCreate(1,  sizeof(SystemStatus));
    fanCmdQueue   = xQueueCreate(5,  sizeof(FanCommand));
    setpointQueue = xQueueCreate(5,  sizeof(SetpointRequest));
    saveQueue     = xQueueCreate(5,  sizeof(SaveRequest));
    inputQueue    = xQueueCreate(16, sizeof(InputEvent));
    netStatus     = xQueueCreate(1,  sizeof(NetStatus));

    // Names show up in the CLion RTOS view.
    vQueueAddToRegistry(sensorData,    "sensorData");
    vQueueAddToRegistry(systemStatus,  "systemStatus");
    vQueueAddToRegistry(fanCmdQueue,   "fanCmdQueue");
    vQueueAddToRegistry(setpointQueue, "setpointQueue");
    vQueueAddToRegistry(saveQueue,     "saveQueue");
    vQueueAddToRegistry(inputQueue,    "inputQueue");
    vQueueAddToRegistry(netStatus,     "netStatus");

    return sensorData && systemStatus && fanCmdQueue && setpointQueue && saveQueue && inputQueue && netStatus;
}
