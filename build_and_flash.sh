#!/usr/bin/env bash
#
# Build and flash STM32F103 AHT20 Baremetal Project (Linux)
#
set -euo pipefail

# Directory of this script = project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PROJECT_NAME="stm32f103_aht20_baremetal"
BUILD_DIR="build"
STLINK_SN="54FF72064885525637190787"
STM32_PROGRAMMER_CLI="${STM32_PRG_PATH:-}/STM32_Programmer_CLI"

echo "==============================================="
echo "Building STM32F103 AHT20 Baremetal Project"
echo "==============================================="

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    echo "Error: arm-none-eabi-gcc not found in PATH."
    echo "Install it with: sudo apt install gcc-arm-none-eabi"
    exit 1
fi

if ! command -v "$STM32_PROGRAMMER_CLI" >/dev/null 2>&1; then
    echo "Error: STM32_Programmer_CLI not found."
    echo "Make sure STM32_PRG_PATH is set and exported to PATH (see ~/.bashrc)."
    exit 1
fi

# Clean build directory
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Configure with CMake
echo
echo "[1/3] Configuring..."
cmake -B "$BUILD_DIR" -G Ninja

# Build
echo
echo "[2/3] Building..."
cmake --build "$BUILD_DIR"

# Summary
echo
echo "[3/3] Build Summary"
echo "==============================================="
if compgen -G "$BUILD_DIR"/*.hex > /dev/null; then
    ls -la "$BUILD_DIR"/*.hex
else
    echo "No .hex file generated!"
fi

if compgen -G "$BUILD_DIR"/*.bin > /dev/null; then
    ls -la "$BUILD_DIR"/*.bin
else
    echo "No .bin file generated!"
fi

echo "==============================================="
echo "Build completed successfully!"
echo

# Flash
echo "Flashing via ST-Link..."
"$STM32_PROGRAMMER_CLI" -c port=SWD sn="$STLINK_SN" -w "$BUILD_DIR/${PROJECT_NAME}.bin" 0x08000000 -v -rst
