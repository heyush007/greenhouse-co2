// Entry point for each task; one .cpp file implements each one.
#pragma once
#include "shared_types.h"

void modbus_task(void *param);   // reads sensors and drives the fan over Modbus
void control_task(void *param);  // decides the valve and fan from the readings
void ui_task(void *param);       // OLED display, encoder and buttons
void storage_task(void *param);  // saves and loads settings in EEPROM
void network_task(void *param);  // ThingSpeak reporting and remote setpoint
void console_task(void *param);  // UART command line
