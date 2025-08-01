# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(TarefaDataLogger C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_subdirectory(lib/FatFs_SPI)   

add_executable(TarefaDataLogger TarefaDataLogger.c 
                                lib/mpu6050.c
                                lib/ssd1306.c
                                hw_config.c
                                lib/sd_config.c)

pico_set_program_name(TarefaDataLogger "TarefaDataLogger")
pico_set_program_version(TarefaDataLogger "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(TarefaDataLogger 1)
pico_enable_stdio_usb(TarefaDataLogger 1)

# Add the standard library to the build
target_link_libraries(TarefaDataLogger
        pico_stdlib
        hardware_i2c
        hardware_pwm
        FatFs_SPI
        hardware_clocks)

# Add the standard include files to the build
target_include_directories(TarefaDataLogger PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(TarefaDataLogger 
        
        )

pico_add_extra_outputs(TarefaDataLogger)

