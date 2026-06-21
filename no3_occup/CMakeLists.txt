# no3_occup/CMakeLists.txt
add_executable(no3_occup
    src/main.c
    src/lorawan_task.c
)
target_include_directories(no3_occup PRIVATE
    ${CMAKE_SOURCE_DIR}               # FreeRTOSConfig.h
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib/sensors   # headers dos sensores
    ${PICO_LORAWAN_PATH}/src
    ${PICO_LORAWAN_PATH}/src/mac
    ${PICO_LORAWAN_PATH}/src/system
    ${PICO_LORAWAN_PATH}/src/radio
    ${PICO_LORAWAN_PATH}/src/boards
)
target_link_libraries(no3_occup
    pico_stdlib pico_multicore pico_lorawan
    hardware_spi hardware_gpio
    sensors FreeRTOS-Kernel-Heap4 freertos_config
)

add_definitions(-DCONTEXT_MANAGEMENT_ENABLED=0)
target_compile_definitions(no3_occup PRIVATE DEBUG)

pico_enable_stdio_usb(no3_occup 1)
pico_enable_stdio_uart(no3_occup 1)
pico_add_extra_outputs(no3_occup)