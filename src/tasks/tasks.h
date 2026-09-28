// Task entry points. One file per task; one owner (person) per file.
#pragma once
#include "shared_types.h"

void modbus_task(void *param);   // owns UART1 Modbus bus
void control_task(void *param);  // owns GPIO27 valve; decides fan speed
void ui_task(void *param);       // owns I2C1 (OLED); handles inputQueue
void storage_task(void *param);  // owns I2C0 (EEPROM) and the Settings copy; param: Settings* loaded at boot
void network_task(void *param);  // optional; param: const Settings* snapshot for Wi-Fi credentials
