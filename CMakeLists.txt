# smartcampus/CMakeLists.txt
cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(smartcampus C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

# Região AU915 — Brasil
add_definitions(-DREGION_AU915)
add_definitions(-DACTIVE_REGION=LORAMAC_REGION_AU915)
add_definitions(-DLORAMAC_CLASS_A)

# pico-lorawan — clone único no computador
if(NOT DEFINED ENV{PICO_LORAWAN_PATH})
    message(FATAL_ERROR
        "PICO_LORAWAN_PATH não definido.\n"
        "  git clone --recurse-submodules "
        "https://github.com/sandeepmistry/pico-lorawan.git ~/pico-lorawan\n"
        "  export PICO_LORAWAN_PATH=~/pico-lorawan")
endif()
set(PICO_LORAWAN_PATH $ENV{PICO_LORAWAN_PATH})
add_subdirectory(${PICO_LORAWAN_PATH} pico-lorawan)

# FreeRTOS config (raiz = onde está o FreeRTOSConfig.h)
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE ${CMAKE_CURRENT_LIST_DIR})

# Biblioteca de sensores — definida uma vez, usada por todos
add_library(sensors
    lib/sensors/dht11.c
    lib/sensors/ir_raw.c
    lib/sensors/relay.c
    lib/sensors/vl53l0x.c
    lib/sensors/spi_diagnostic.c
)
target_include_directories(sensors PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/lib/sensors
)
target_link_libraries(sensors
    pico_stdlib
    hardware_gpio
    hardware_spi
    hardware_i2c
    hardware_sync
    freertos_config
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
)

# Subprojetosr
add_subdirectory(no1_temp)
add_subdirectory(no2_relay)
add_subdirectory(no3_occup)
add_subdirectory(tests)