# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD none CACHE STRING "Board type")


# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(CC3501_RaspberryRadio C CXX ASM)

# The board uses a Winbond W25Q16. Use its Quad I/O XIP boot2 rather than
# the generic 0x03-read boot2, which is limited to a much lower read clock.
set(PICO_DEFAULT_BOOT_STAGE2 boot2_w25q080 CACHE STRING
        "Winbond W25Q Quad I/O boot stage 2" FORCE)
# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# RP2040 XIP QSPI Flash clock = clk_sys / PICO_FLASH_SPI_CLKDIV.
# With the project clock at 192 MHz, a divider of 2 gives the W25Q16 a 96 MHz SCK.
# This is a boot2-only option and must not be changed at run time while executing from Flash.
target_compile_definitions(bs2_default PRIVATE PICO_FLASH_SPI_CLKDIV=2)

# Build the vendored FreeRTOS Kernel with its official RP2040 GCC port.
set(FREERTOS_KERNEL_PATH
        ${CMAKE_CURRENT_LIST_DIR}/src/OS/FreeRTOS-Kernel
        CACHE PATH "Vendored FreeRTOS Kernel path" FORCE)
set(FREERTOS_CONFIG_FILE_DIRECTORY
        ${CMAKE_CURRENT_LIST_DIR}/src/OS
        CACHE PATH "Project FreeRTOS configuration path" FORCE)
add_subdirectory(
        ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040
        freertos_kernel)

# Build the complete vendored CMSIS-DSP library for RP2040 (Cortex-M0+).
set(CMSISCORE ${CMAKE_CURRENT_LIST_DIR}/src/CMSIS_DSP/CMSIS/Core)
set(DISABLEFLOAT16 ON CACHE BOOL "Disable unsupported float16 kernels" FORCE)
set(LOOPUNROLL OFF CACHE BOOL "Use compiler-managed loop unrolling" FORCE)
set(NEON OFF CACHE BOOL "RP2040 has no NEON" FORCE)
set(NEONEXPERIMENTAL OFF CACHE BOOL "RP2040 has no NEON" FORCE)
set(MVEI OFF CACHE BOOL "RP2040 has no MVE" FORCE)
set(MVEF OFF CACHE BOOL "RP2040 has no MVE" FORCE)
set(HELIUM OFF CACHE BOOL "RP2040 has no Helium" FORCE)

add_subdirectory(src/CMSIS_DSP/Source cmsis_dsp)

target_compile_options(CMSISDSP PRIVATE
    -ffunction-sections
    -fdata-sections
)

target_compile_definitions(CMSISDSP PUBLIC
ARM_MATH_CM0PLUS
DISABLEFLOAT16
)

# Build the complete vendored LVGL library and all v9.5.0 demo sources.
set(LV_BUILD_CONF_PATH
${CMAKE_CURRENT_LIST_DIR}/src/GUI/lv_conf.h
CACHE PATH "Project LVGL configuration" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "Build all LVGL demos" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "Do not build LVGL examples" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "Build internal ThorVG" FORCE)

add_subdirectory(src/GUI/lvgl lvgl)

# target_compile_options(lvgl_demos PRIVATE -ffunction-sections -fdata-sections)

target_compile_options(lvgl PRIVATE
-ffunction-sections
-fdata-sections
)

if(TARGET lvgl_thorvg)
target_compile_options(lvgl_thorvg PRIVATE
-ffunction-sections
-fdata-sections
)
endif()

# Add executable. Default name is the project name, version 0.1

add_executable(CC3501_RaspberryRadio 
${CMAKE_CURRENT_LIST_DIR}/src/main/main.c

${CMAKE_CURRENT_LIST_DIR}/src/peripheral/adc/RP2040_adc.c
${CMAKE_CURRENT_LIST_DIR}/src/peripheral/pwm/RP2040_pwm.c
${CMAKE_CURRENT_LIST_DIR}/src/peripheral/adc_dma_sampling/adc_dma_capture.c
${CMAKE_CURRENT_LIST_DIR}/src/peripheral/clock/RP2040_clk_init.c

${CMAKE_CURRENT_LIST_DIR}/src/driver/NS4160/NS4160.c
${CMAKE_CURRENT_LIST_DIR}/src/driver/CST816D/CST816D.c
${CMAKE_CURRENT_LIST_DIR}/src/driver/GPS_module/GPS_nmea_parser.c
${CMAKE_CURRENT_LIST_DIR}/src/driver/GPS_module/GPS_module.c
${CMAKE_CURRENT_LIST_DIR}/src/driver/SI4732/SI4732.c
${CMAKE_CURRENT_LIST_DIR}/src/driver/ST7789/ST7789_LCD.c

${CMAKE_CURRENT_LIST_DIR}/src/BSP/board_init.c


)

pico_set_program_name(CC3501_RaspberryRadio "CC3501_RaspberryRadio")
pico_set_program_version(CC3501_RaspberryRadio "0.1")


# Modify the below lines to enable/disable output over UART/USB
# Route Pico SDK standard I/O through UART0 on GPIO28/GPIO29.
target_compile_definitions(CC3501_RaspberryRadio PRIVATE
        PICO_DEFAULT_UART=0
        PICO_DEFAULT_UART_TX_PIN=28
        PICO_DEFAULT_UART_RX_PIN=29
        PICO_DEFAULT_UART_BAUD_RATE=2000000
)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(CC3501_RaspberryRadio 1)
pico_enable_stdio_usb(CC3501_RaspberryRadio 1)

# Add the standard library to the build
target_link_libraries(CC3501_RaspberryRadio
        FreeRTOS-Kernel-Heap4
        lvgl::lvgl
        CMSISDSP
        pico_stdlib
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_interp
        hardware_timer
        hardware_watchdog
        hardware_clocks
        hardware_pll
        hardware_uart
        hardware_gpio
        hardware_vreg
        hardware_pwm
        hardware_adc
        hardware_irq
        )

# Add the standard include files to the build
target_include_directories(CC3501_RaspberryRadio PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/peripheral/adc_dma_sampling
        ${CMAKE_CURRENT_LIST_DIR}/src/peripheral/clock
        ${CMAKE_CURRENT_LIST_DIR}/src/peripheral/pwm
        ${CMAKE_CURRENT_LIST_DIR}/src/peripheral/adc
        ${CMAKE_CURRENT_LIST_DIR}/src/OS
        ${CMAKE_CURRENT_LIST_DIR}/src/main
        ${CMAKE_CURRENT_LIST_DIR}/src/GUI
        ${CMAKE_CURRENT_LIST_DIR}/src/BSP
        ${CMAKE_CURRENT_LIST_DIR}/src/driver/GPS_module
        ${CMAKE_CURRENT_LIST_DIR}/src/driver/SI4732
        ${CMAKE_CURRENT_LIST_DIR}/src/driver/ST7789
        ${CMAKE_CURRENT_LIST_DIR}/src/driver/NS4160
        ${CMAKE_CURRENT_LIST_DIR}/src/driver/CST816D
)

pico_add_extra_outputs(CC3501_RaspberryRadio)

