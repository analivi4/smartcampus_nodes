# no2_relay/CMakeLists.txt
add_executable(no2_relay
    src/main.c
    src/lorawan_task.c
)
target_include_directories(no2_relay PRIVATE
    ${CMAKE_SOURCE_DIR}               # FreeRTOSConfig.h
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib/sensors   # headers dos sensores
    ${PICO_LORAWAN_PATH}/src
    ${PICO_LORAWAN_PATH}/src/mac
    ${PICO_LORAWAN_PATH}/src/system
    ${PICO_LORAWAN_PATH}/src/radio
    ${PICO_LORAWAN_PATH}/src/boards
)
target_link_libraries(no2_relay
    pico_stdlib pico_multicore pico_lorawan
    hardware_spi hardware_gpio
    sensors FreeRTOS-Kernel-Heap4 freertos_config
)

add_definitions(-DCONTEXT_MANAGEMENT_ENABLED=0)
target_compile_definitions(no2_relay PRIVATE DEBUG)

pico_enable_stdio_usb(no2_relay 1)
pico_enable_stdio_uart(no2_relay 1)
pico_add_extra_outputs(no2_relay)