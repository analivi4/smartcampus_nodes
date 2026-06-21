# tests/CMakeLists.txt

# Função auxiliar para não repetir o bloco todo
function(add_test_firmware name)
    add_executable(${name} ${name}/main.c)
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/lib/sensors
        ${PICO_LORAWAN_PATH}/src
        ${PICO_LORAWAN_PATH}/src/mac
        ${PICO_LORAWAN_PATH}/src/system
        ${PICO_LORAWAN_PATH}/src/radio
        ${PICO_LORAWAN_PATH}/src/boards
    )
    target_link_libraries(${name}
        pico_stdlib pico_multicore pico_lorawan
        hardware_spi hardware_gpio hardware_i2c
        hardware_sync sensors FreeRTOS-Kernel-Heap4 freertos_config
    )
    pico_enable_stdio_usb(${name} 1)
    pico_enable_stdio_uart(${name} 1)
    pico_add_extra_outputs(${name})
endfunction()

add_test_firmware(test_radio)
add_test_firmware(test_stack)
add_test_firmware(test_dht11)
add_test_firmware(test_vl53l0x)
add_test_firmware(test_relay)
add_test_firmware(test_ir)