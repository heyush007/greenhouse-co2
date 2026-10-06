# Every .cpp under src/ (main.cpp, app/, drivers/, tasks/) is compiled automatically.
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

add_executable(${ProjectName}
        ${APP_SOURCES}
)

target_include_directories(${ProjectName} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/app
        ${CMAKE_CURRENT_LIST_DIR}/drivers
        ${CMAKE_CURRENT_LIST_DIR}/tasks
        ${CMAKE_CURRENT_LIST_DIR}/display
        ${CMAKE_CURRENT_LIST_DIR}/net
)

# --- Wi-Fi + ThingSpeak secrets (never committed) --------------------------
# Defaults come from environment variables. If a local, git-ignored
# secrets.cmake exists it overrides them - copy secrets.cmake.example to
# secrets.cmake and fill in your values. secrets.cmake is in .gitignore.
set(WIFI_SSID               "$ENV{WIFI_SSID}")
set(WIFI_PASSWORD           "$ENV{WIFI_PASSWORD}")
set(THINGSPEAK_HOST         "$ENV{THINGSPEAK_HOST}")
set(THINGSPEAK_API_KEY      "$ENV{THINGSPEAK_API_KEY}")
set(THINGSPEAK_TALKBACK_KEY "$ENV{THINGSPEAK_TALKBACK_KEY}")
set(THINGSPEAK_TALKBACK_ID  "$ENV{THINGSPEAK_TALKBACK_ID}")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/secrets.cmake")
        include("${CMAKE_CURRENT_LIST_DIR}/secrets.cmake")
        message(STATUS "Loaded Wi-Fi/ThingSpeak credentials from secrets.cmake")
endif()

target_compile_definitions(${ProjectName} PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        THINGSPEAK_HOST=\"${THINGSPEAK_HOST}\"
        THINGSPEAK_API_KEY=\"${THINGSPEAK_API_KEY}\"
        THINGSPEAK_TALKBACK_KEY=\"${THINGSPEAK_TALKBACK_KEY}\"
        THINGSPEAK_TALKBACK_ID=\"${THINGSPEAK_TALKBACK_ID}\"
        NO_SYS=0
        PICO_CYW43_ARCH_DEFAULT_COUNTRY_CODE=CYW43_COUNTRY_FINLAND
)

target_link_libraries(${ProjectName}
        pico_stdlib
        hardware_i2c
        hardware_gpio
        FreeRTOS-Kernel-Heap4
        pico_cyw43_arch_lwip_sys_freertos
)

pico_add_extra_outputs(${ProjectName})

# Disable usb output, enable uart output (debug prints via the PicoProbe)
pico_enable_stdio_usb(${ProjectName} 0)
pico_enable_stdio_uart(${ProjectName} 1)