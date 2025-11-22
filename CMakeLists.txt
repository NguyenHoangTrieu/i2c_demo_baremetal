cmake_minimum_required(VERSION 3.22)

#
# Baremetal STM32F103 AHT20 I2C/UART Project
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME stm32f103_aht20_baremetal)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Baremetal project defines
set(BAREMETAL_Defines_Syms 
    STM32F103xB
    USE_FULL_ASSERT
    HSI_VALUE=8000000
    $<$<CONFIG:Debug>:DEBUG>
)

# Baremetal include paths (only need current directory)
set(BAREMETAL_Include_Dirs
    ${CMAKE_SOURCE_DIR}
)

# Baremetal application sources
set(BAREMETAL_Application_Src
    ${CMAKE_SOURCE_DIR}/main.c
    ${CMAKE_SOURCE_DIR}/i2c_handler.c
    ${CMAKE_SOURCE_DIR}/uart_handler.c
    ${CMAKE_SOURCE_DIR}/startup_stm32f103xb.s
    ${CMAKE_SOURCE_DIR}/system_stm32f1xx.c
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${BAREMETAL_Application_Src}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${BAREMETAL_Include_Dirs}
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    ${BAREMETAL_Defines_Syms}
)

# Post-build commands to generate hex and bin files
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMENT "Building HEX and BIN files"
)

# Print executable size
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMENT "Program size:"
)

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
    ADDITIONAL_CLEAN_FILES "${CMAKE_PROJECT_NAME}.map;${CMAKE_PROJECT_NAME}.hex;${CMAKE_PROJECT_NAME}.bin"
)

# Validate that code is compatible with C standard
if((CMAKE_C_STANDARD LESS 11))
    message(FATAL_ERROR "Generated code requires C11 or higher")
endif()
