// Load/save the settings kept in EEPROM.
#pragma once
#include "shared_types.h"

Settings settings_defaults();                // factory defaults
void     settings_seal(Settings &s);         // set magic, version and crc
bool     settings_valid(const Settings &s);  // check magic and checksum
bool     settings_load(Settings &out);       // read from EEPROM, else defaults
bool     settings_save(const Settings &s);   // write to EEPROM
