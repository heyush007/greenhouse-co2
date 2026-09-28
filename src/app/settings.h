// Persistent settings: defaults, validation, EEPROM load/save.
#pragma once
#include "shared_types.h"

Settings settings_defaults();
void     settings_seal(Settings &s);                 // sets magic, version and crc
bool     settings_valid(const Settings &s);
bool     settings_load(Settings &out);               // false -> out holds defaults
bool     settings_save(const Settings &s);
