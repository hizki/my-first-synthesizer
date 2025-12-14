#!/bin/bash

# ESP32 Chord Synth - Quick Upload Script
# Compiles and uploads the sketch to ESP32

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}ESP32 Chord Synth - Upload Script${NC}"
echo "===================================="
echo ""

# Find the Arduino CLI or IDE
if command -v arduino-cli &> /dev/null; then
    ARDUINO_CMD="arduino-cli"
    echo -e "${GREEN}✓${NC} Found arduino-cli"
elif [ -f "/Applications/Arduino.app/Contents/MacOS/Arduino" ]; then
    ARDUINO_CMD="/Applications/Arduino.app/Contents/MacOS/Arduino"
    echo -e "${GREEN}✓${NC} Found Arduino IDE"
else
    echo -e "${RED}✗${NC} Arduino CLI or IDE not found"
    echo "Please install Arduino CLI or Arduino IDE"
    exit 1
fi

# Auto-detect ESP32 port (macOS/Linux)
PORT=$(ls /dev/cu.usbserial* 2>/dev/null | head -n 1)
if [ -z "$PORT" ]; then
    PORT=$(ls /dev/ttyUSB* 2>/dev/null | head -n 1)
fi

if [ -z "$PORT" ]; then
    echo -e "${YELLOW}⚠${NC}  Could not auto-detect ESP32 port"
    echo "Available ports:"
    ls /dev/cu.* /dev/ttyUSB* 2>/dev/null
    echo ""
    read -p "Enter port (e.g., /dev/cu.usbserial-0001): " PORT
fi

echo -e "${GREEN}✓${NC} Using port: $PORT"
echo ""

# Board configuration
FQBN="esp32:esp32:esp32"
UPLOAD_SPEED="115200"  # Slower speed for more reliable uploads

if [ "$ARDUINO_CMD" = "arduino-cli" ]; then
    echo "Compiling and uploading with arduino-cli (baud rate: $UPLOAD_SPEED)..."
    echo ""
    arduino-cli compile --fqbn $FQBN . && \
    arduino-cli upload --fqbn $FQBN --port $PORT --upload-property upload.speed=$UPLOAD_SPEED .
else
    echo "Compiling and uploading with Arduino IDE..."
    echo ""
    $ARDUINO_CMD --board $FQBN --port $PORT --upload chord-synth.ino
fi

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ Upload successful!${NC}"
    echo ""
    echo "The ESP32 Chord Synth should now be playing a jazz progression"
    echo "and displaying the waveform on the OLED screen."
    echo ""
    echo "Controls:"
    echo "  DIAL1 (GPIO 4): Volume control"
    echo "  DIAL2 (GPIO 33): Unison voices (x1/x2/x3/x4)"
    echo "  BOOT button: Short press = waveform, Long press = mode"
    echo ""
    echo "To monitor serial output:"
    echo "  screen $PORT 115200"
else
    echo ""
    echo -e "${RED}✗ Upload failed${NC}"
    exit 1
fi

