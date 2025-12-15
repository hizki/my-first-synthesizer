#!/bin/bash

# Build script for Wokwi simulation
# Compiles the Arduino sketch into a binary that can be run in Wokwi

set -e

echo "=========================================="
echo "Building Chord Synth for Wokwi"
echo "=========================================="

# Check if arduino-cli is installed
if ! command -v arduino-cli &> /dev/null; then
    echo "ERROR: arduino-cli is not installed"
    echo "Install it from: https://arduino.github.io/arduino-cli/latest/installation/"
    exit 1
fi

# Project name
PROJECT_NAME="chord-synth"

# Ensure build directory exists
mkdir -p build

echo ""
echo "Step 1: Compiling sketch..."
arduino-cli compile \
    --fqbn esp32:esp32:esp32 \
    --output-dir build \
    --build-property "build.partitions=default" \
    --build-property "upload.maximum_size=1310720" \
    ${PROJECT_NAME}.ino

echo ""
echo "Step 2: Build complete!"
echo ""
echo "Generated files:"
ls -lh build/*.bin build/*.elf 2>/dev/null || true

echo ""
echo "=========================================="
echo "Build successful!"
echo "=========================================="
echo ""
echo "To run in Wokwi:"
echo "1. Install Wokwi VS Code extension"
echo "2. Press F1 and select 'Wokwi: Start Simulator'"
echo "OR"
echo "Upload this project folder to wokwi.com"
echo ""
echo "Note: Audio output (MAX98357A) is not simulated in Wokwi."
echo "The synthesizer logic will run, but you won't hear sound."
echo "Use this simulation to test:"
echo "  - OLED display output"
echo "  - Volume control (DIAL1/pot1)"
echo "  - Unison control (DIAL2/pot2)"
echo "  - Mode/waveform switching (BOOT button - GPIO 0)"
echo ""

