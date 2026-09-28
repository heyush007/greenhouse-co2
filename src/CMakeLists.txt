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
)

target_compile_definitions(${ProjectName} PRIVATE
)

target_link_libraries(${ProjectName}
        pico_stdlib
        hardware_i2c
        hardware_gpio
        FreeRTOS-Kernel-Heap4
)

pico_add_extra_outputs(${ProjectName})

# Disable usb output, enable uart output (debug prints via the PicoProbe)
pico_enable_stdio_usb(${ProjectName} 0)
pico_enable_stdio_uart(${ProjectName} 1)