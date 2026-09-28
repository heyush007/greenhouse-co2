# Greenhouse CO₂ controller

Embedded systems capstone (Metropolia). A Raspberry Pi Pico running FreeRTOS keeps the
CO₂ level of a greenhouse at a user-set value. It reads the Vaisala GMP252 (CO₂) and
HMP60 (RH/T) sensors over Modbus RTU, doses CO₂ through a valve on GPIO27, and vents with
a fan driven by a Produal MIO 12-V module.

- **Setup and team workflow:** [SETUP.md](SETUP.md)
- **System diagram:** [docs/system_diagram.png](docs/system_diagram.png) (editable: `docs/greenhouse_system_diagram.drawio`)

## Tasks

| Task | Priority | Owns | Job |
|---|---|---|---|
| `control_task` | 4 | GPIO27 valve | Keeps CO₂ at the setpoint; vents above 2000 ppm |
| `modbus_task` | 3 | UART1 Modbus bus | Reads sensors every 2 s, writes fan speed |
| `ui_task` | 2 | I2C1 OLED | Shows values, edits the setpoint with the encoder |
| `storage_task` | 1 | I2C0 EEPROM | Saves settings |
| `network_task` | 1 | Wi-Fi | Optional: ThingSpeak (off by default) |

Each peripheral has exactly one owner task, so the design needs no mutex. Tasks talk only
through the queues in `src/app/app_queues.h`, carrying the types in `src/app/shared_types.h`.

## Source layout

```
src/
  main.cpp              boot: load settings, create queues, start tasks
  app/
    config.h            every pin, address and tuning constant
    shared_types.h      message structs shared by all tasks (the team contract)
    app_queues.*        the queues and mailboxes from the diagram
    settings.*          EEPROM settings with CRC check and defaults
  drivers/
    PicoOsUart.*        course-provided interrupt-driven UART
    ModbusRtu.*         Modbus RTU master (functions 03, 04, 06)
    Eeprom.*            24Cxx EEPROM on I2C0
    InputIsr.*          GPIO ISR: encoder + buttons -> inputQueue
  tasks/
    tasks.h             task entry points
    *_task.cpp          one file per task
docs/                   diagrams and documentation for the submission
```

## Debug output

UART0 (the debug probe's serial port, 115200 baud). Each task prefixes its lines:
`[modbus]`, `[control]`, `[ui]`, `[storage]`, `[network]`.
