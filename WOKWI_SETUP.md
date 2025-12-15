# Wokwi Simulation Setup for Chord Synth

This guide will help you run the Chord Synth project in the Wokwi simulator.

## What is Wokwi?

Wokwi is an online electronics simulator that lets you test ESP32 projects without physical hardware. It simulates the microcontroller, display, and input controls.

## What's Simulated

✅ **Fully Simulated:**
- ESP32-WROOM-32 microcontroller
- SSD1306 OLED display (128x64)
- Volume potentiometer (DIAL1 on GPIO 4)
- Unison potentiometer (DIAL2 on GPIO 33)
- BOOT button (GPIO 0) - built-in on ESP32

❌ **Not Simulated:**
- MAX98357A I2S audio amplifier
- Actual audio output (speakers)

The code will run completely, including all I2S audio generation, but you won't hear sound. Use Wokwi to test the display, controls, and logic.

## Hardware Configuration

The `diagram.json` file defines the following connections:

### OLED Display (SSD1306)
- **VCC** → ESP32 3.3V
- **GND** → ESP32 GND
- **SDA** → GPIO 21
- **SCL** → GPIO 19

### Potentiometer 1 (Volume - DIAL1)
- **VCC** → ESP32 3.3V
- **GND** → ESP32 GND
- **SIG** → GPIO 4

### Potentiometer 2 (Unison - DIAL2)
- **VCC** → ESP32 3.3V
- **GND** → ESP32 GND
- **SIG** → GPIO 33

### BOOT Button
- Built-in on GPIO 0 (no wiring needed)
- **Short press (<1s):** Cycle waveform (SAW → SQR → TRI → SIN)
- **Long press (≥1s):** Cycle mode (PROGRESSION → CHORD → NOTE)

### OK Button (Green)
- **GPIO 13** → One side of button
- **GND** → Other side of button
- **Function:** Cycle waveform (same as short press on BOOT)

### BACK Button (Red)
- **GPIO 16** → One side of button
- **GND** → Other side of button
- **Function:** Cycle mode (same as long press on BOOT)

## Method 1: VS Code Extension (Recommended)

### Prerequisites
1. Install [VS Code](https://code.visualstudio.com/)
2. Install the [Wokwi VS Code extension](https://marketplace.visualstudio.com/items?itemName=wokwi.wokwi-vscode)
3. Install [arduino-cli](https://arduino.github.io/arduino-cli/latest/installation/)

### Steps

1. **Build the firmware:**
   ```bash
   cd projects/chord-synth
   ./build-wokwi.sh
   ```

   This compiles the sketch and creates:
   - `build/chord-synth.ino.bin` (firmware)
   - `build/chord-synth.ino.elf` (debug symbols)

2. **Start the simulator:**
   - Press `F1` in VS Code
   - Type and select: `Wokwi: Start Simulator`
   - The simulator will load the firmware and start running

3. **Interact with the simulation:**
   - Drag the potentiometer sliders to adjust volume and unison
   - Click the BOOT button for short/long presses
   - Watch the OLED display update in real-time

## Method 2: Wokwi.com (Online)

### Prerequisites
1. A [Wokwi.com](https://wokwi.com/) account (free)
2. Pre-built firmware files

### Steps

1. **Build the firmware** (on your local machine):
   ```bash
   cd projects/chord-synth
   ./build-wokwi.sh
   ```

2. **Create a new project on Wokwi.com:**
   - Go to [wokwi.com/projects/new](https://wokwi.com/projects/new/esp32)
   - Select "ESP32" as the board

3. **Upload files:**
   - Upload `diagram.json`
   - Upload `wokwi.toml`
   - Upload `build/chord-synth.ino.bin`
   - Upload `build/chord-synth.ino.elf`

4. **Start the simulation:**
   - Click the green "Start Simulation" button
   - Interact with the virtual hardware

## Troubleshooting

### Build Fails
- **arduino-cli not found:** Install from [https://arduino.github.io/arduino-cli/latest/installation/](https://arduino.github.io/arduino-cli/latest/installation/)
- **ESP32 core missing:** Run `arduino-cli core install esp32:esp32`
- **Library missing:** Install required libraries:
  ```bash
  arduino-cli lib install "Adafruit GFX Library"
  arduino-cli lib install "Adafruit SSD1306"
  ```

### Display Not Working
- Check that `diagram.json` has correct I2C address (0x3C)
- Verify connections: SDA=GPIO21, SCL=GPIO19

### Potentiometers Not Responding
- Make sure `diagram.json` has correct GPIO pins:
  - DIAL1 (volume) = GPIO 4
  - DIAL2 (unison) = GPIO 33

### BOOT Button Not Working
- In Wokwi, click and hold the button to simulate long press
- Quick click for short press

## Testing Checklist

Use this checklist to verify all features work in simulation:

- [ ] Display shows boot animation on startup
- [ ] Volume bar responds to pot1 (DIAL1)
- [ ] Waveform animation plays when cycling waveforms
- [ ] Short press cycles: SAW → SQR → TRI → SIN
- [ ] Long press cycles: PROGRESSION → CHORD → NOTE
- [ ] In chord modes, pot2 (DIAL2) changes unison (x1→x2→x3→x4)
- [ ] Progression mode auto-advances through chords
- [ ] Serial monitor shows debug messages

## Limitations

Remember that Wokwi cannot simulate:
- Actual audio output (I2S data generation happens but no sound)
- Real-time audio performance characteristics
- Speaker/amplifier hardware behavior

For audio testing, you'll need to upload to real hardware using the `upload.sh` script.

## Resources

- [Wokwi Documentation](https://docs.wokwi.com/)
- [ESP32 Wokwi Examples](https://docs.wokwi.com/guides/esp32)
- [arduino-cli Documentation](https://arduino.github.io/arduino-cli/latest/)

## Next Steps

After testing in Wokwi, upload to real hardware:
```bash
./upload.sh
```

This will give you actual audio output through the MAX98357A amplifier.

