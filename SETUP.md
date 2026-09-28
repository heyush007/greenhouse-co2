# Setup and team workflow

Do the phases in order. Each step ends with a **Check**: don't move on until it passes.

---

## Phase 0: Decide as a group (15 min, before anyone touches GitLab)

1. **Template repo.** The project spec names `lansk/rp2040-freertos`; the course setup guide
   uses `lansk/rp2040-freertos-minimal`. Ask the teacher which one to build on.
2. **Scope.** Minimum to pass: local UI, persistent setpoint, all sensors read and shown,
   documentation. Decide now whether you also do ThingSpeak (Network task).
3. **Owners.** One person per task file:

   | Area | Files | Owner |
   |---|---|---|
   | GitLab + build integration | CMakeLists, repo settings | |
   | Modbus task | `modbus_task.cpp`, `ModbusRtu.*` | |
   | Control task | `control_task.cpp` | |
   | UI + input | `ui_task.cpp`, `InputIsr.*`, OLED driver | |
   | Storage + docs lead | `storage_task.cpp`, `settings.*`, `docs/` | |

4. **Box time.** There is one test box. Agree who has it and when.

---

## Phase 1: Create the GitLab project (one person, about 30 min)

### 1.1 Create an empty project
1. Go to `gitlab.metropolia.fi` → **New project** → **Create blank project**.
2. Name: `greenhouse-co2`. Visibility: **Private**.
3. **Uncheck** "Initialize repository with a README" (you'll push the template into it).
4. **Create project**.

**Check:** the project page shows "The repository for this project is empty" and push instructions.

### 1.2 Add the group and the teacher
1. Left menu → **Manage → Members** → **Invite members**.
2. Add each groupmate as **Maintainer**, or **Developer** if one person should control merges.
3. Ask the teacher which role they want (usually Reporter) and add them.

**Check:** every member can open the project page.

### 1.3 Push the course template
```bash
git clone --recurse-submodules https://gitlab.metropolia.fi/lansk/<TEMPLATE>.git greenhouse-co2
cd greenhouse-co2
git remote rename origin upstream          # keeps the teacher's repo for later fixes
git remote add origin https://gitlab.metropolia.fi/<YOUR-USER>/greenhouse-co2.git
git branch                                 # note the branch name: main or master
git push -u origin --all
```
- If `git submodule status` shows lines starting with `-`, the submodules are empty. Run
  `git submodule update --init --recursive`.

**Check:** GitLab shows the template files, and CLion builds the template unchanged.

### 1.4 Add this starter code
1. Copy `src/`, `docs/`, `.gitignore`, `README.md` and `SETUP.md` from this pack into the repo root.
   If the template already has a `.gitignore`, merge the two.
2. Remove the template's own `main.cpp` from the build (ours replaces it).
3. If the template already contains `PicoOsUart.h/.cpp`, delete our copies in `src/drivers/`.
   Otherwise the linker reports duplicate symbols.
4. In the template's `CMakeLists.txt`, after `add_executable(...)`, add:
   ```cmake
   file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
   target_sources(${PROJECT_NAME} PRIVATE ${APP_SOURCES})
   target_include_directories(${PROJECT_NAME} PRIVATE
       src src/app src/drivers src/tasks)
   target_link_libraries(${PROJECT_NAME} hardware_i2c hardware_gpio)
   ```
   Use the executable name the template uses if it isn't `${PROJECT_NAME}`.
5. The code calls `vTaskDelayUntil`, `xTaskGetSchedulerState` and `vQueueAddToRegistry`.
   If the build complains, set these in the template's `FreeRTOSConfig.h`:
   `INCLUDE_xTaskDelayUntil 1`, `INCLUDE_xTaskGetSchedulerState 1`, `configQUEUE_REGISTRY_SIZE 10`.
6. Commit and push:
   ```bash
   git add -A
   git commit -m "Add starter architecture: tasks, queues, Modbus, EEPROM, input ISR"
   git push
   ```

**Check:** clean build in CLion with no errors.

### 1.5 Protect `main`
1. **Settings → Repository → Protected branches**.
2. Branch `main` (or `master`): *Allowed to merge* = **Maintainers**, *Allowed to push and merge* = **No one**.
3. **Settings → Merge requests**: turn on **Enable "Delete source branch" option by default**.

**Check:** `git push` directly to main is rejected.

### 1.6 Create milestones and issues
1. **Plan → Milestones → New milestone**. Create:
   `M1 Setup`, `M2 Sensors on the bus`, `M3 Control loop`, `M4 UI and EEPROM`, `M5 Docs and demo`, `M6 Cloud (optional)`.
2. **Plan → Issues → Import CSV** (the import button is next to "New issue") and upload
   `gitlab_issues.csv` from this pack. Each title starts with its milestone, e.g. `[M2]`.
3. Open each issue: set the milestone, assign an owner.

**Check:** the issue board shows every issue with an owner.

---

## Phase 2: Every member's machine (each person, 30–60 min)

1. Clone the group repo:
   ```bash
   git clone --recurse-submodules https://gitlab.metropolia.fi/<YOUR-USER>/greenhouse-co2.git
   ```
2. Open it in CLion and finish the CMake configuration (course PDF "Setting up FreeRTOS project").
3. **Settings → Build, Execution, Deployment → Embedded Development → RTOS Integration**:
   check both boxes and select **FreeRTOS**. This setting is per project, not global.
4. Wire the debug probe Pico to the controller Pico (course PDF "Debugger wiring").
5. Create the OpenOCD run configuration (course PDF "Pico debugger installation").
6. Build, flash, and set a breakpoint on the first line of `main()`.

**Check:** the debugger stops at the breakpoint. After you continue, the probe's serial port
(115200) prints `--- Greenhouse CO2 controller boot ---`.

---

## Phase 3: First bring-up on the test box (in this order)

The **left** board (blue knob) is the controller. The **right** board (yellow knob) is the
simulator: never flash it or change its wiring. Confirm the left/right assignment against the
spec photo before the first flash.

| # | Do | Expected log | If not |
|---|---|---|---|
| 1 | Power on | `Settings not found, using defaults, setpoint 800 ppm` (first boot) | EEPROM address in `config.h` |
| 2 | Wait 2 s | `[modbus] co2=... rh=... t=... fan_pulses=...` with no `(ERR)` | Pins 4/5, stop bits, simulator powered |
| 3 | Simulator console (115200, no echo): `co2 500` | `[modbus] co2=500` | Wrong Modbus address or register |
| 4 | Leave CO₂ below the setpoint | Valve opens for about 1 s, then stays shut at least 30 s | Check `[control]` lines |
| 5 | Simulator: `co2 2100` | Fan to 100 %, `fan_pulses` > 0, valve stays shut | MIO address 1, AO1 register |
| 6 | Simulator: `co2 700` | Fan back to 0 % once CO₂ ≤ setpoint | |
| 7 | Press knob, rotate, press again | `[control] setpoint ...` and `[storage] settings saved` | Encoder pins, pull-ups |
| 8 | Reset the Pico | `Settings loaded from EEPROM` with the new setpoint | EEPROM page size, address |

If rotation is reversed, swap `RotCW`/`RotCCW` in `InputIsr.cpp`.

---

## Phase 4: Daily workflow (everyone)

1. Pull before you start: `git checkout main && git pull`.
2. One branch per issue: `git checkout -b 12-oled-driver` (issue number first).
3. Commit small, with messages that say what changed: `UI: draw CO2 and setpoint on OLED`.
4. Push and open a merge request that says `Closes #12` in the description.
5. **Another member reviews and merges.** Nobody merges their own MR.
6. Changes to `shared_types.h`, `app_queues.*` or `config.h` affect every task: tell the group before merging.
7. Never commit `build/`, `cmake-build-*` or credentials (`.gitignore` covers these).

---

## Phase 5: Finish the assignment

The spec's minimum requirements, and where each one lives:

| Requirement | Where |
|---|---|
| Local UI with persistent CO₂ setpoint | `ui_task.cpp` + OLED driver, `storage_task.cpp` |
| All sensors read and shown on the UI | `modbus_task.cpp`, `ui_task.cpp` |
| User manual: operating through the UI | `docs/user_manual.md` |
| User manual: remote operation (if done) | `docs/user_manual.md` |
| Program documentation: class/block diagrams, flow charts | `docs/` (system diagram is done) |
| Implementation principles | `docs/implementation.md` |

Advanced features, if time allows: ThingSpeak reporting, TalkBack commands, TLS,
network credentials from the UI, a UART console for settings and factory reset
(`SaveField::FactoryReset` already exists).

### Demo checklist
- [ ] Boots and shows CO₂, RH, T, fan % and setpoint on the OLED
- [ ] Setpoint changed with the knob survives a power cycle
- [ ] Valve pulses about 1 s with at least 30 s between pulses
- [ ] Above 2000 ppm the fan runs until CO₂ is back at the setpoint
- [ ] Fan-stopped detection shown on the display
- [ ] Every group member can explain the task diagram
