# no1_temp/CMakeLists.txt
add_executable(no1_temp
    src/main.c
    src/lorawan_task.c
)
target_include_directories(no1_temp PRIVATE
    ${CMAKE_SOURCE_DIR}               # FreeRTOSConfig.h
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib/sensors   # headers dos sensores
    ${PICO_LORAWAN_PATH}/src
    ${PICO_LORAWAN_PATH}/src/mac
    ${PICO_LORAWAN_PATH}/src/system
    ${PICO_LORAWAN_PATH}/src/radio
    ${PICO_LORAWAN_PATH}/src/boards
)
target_link_libraries(no1_temp
    pico_stdlib pico_multicore pico_lorawan
    hardware_spi hardware_gpio
    sensors FreeRTOS-Kernel-Heap4 freertos_config
)

add_definitions(-DCONTEXT_MANAGEMENT_ENABLED=0)
target_compile_definitions(no1_temp PRIVATE DEBUG)

pico_enable_stdio_usb(no1_temp 1)
pico_enable_stdio_uart(no1_temp 1)
pico_add_extra_outputs(no1_temp)