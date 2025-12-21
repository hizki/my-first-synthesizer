# ESP32 Chord Synth

A polyphonic synthesizer with chord progression playback, unison detuning, and real-time waveform visualization on OLED display.

## 🎵 Features

- **Polyphonic synthesis** - Play multiple notes simultaneously
- **Chord progressions** - Auto-advancing jazz progressions @ 75 BPM
- **4 waveforms** - Sawtooth, Square, Triangle, Sine
- **Unison detuning** - x1/x2/x3/x4 voices for rich sound
- **I2S audio output** - High-quality 44.1kHz stereo via MAX98357A
- **OLED display** - Real-time waveform visualization
- **Dual-core** - Audio on Core 1, Display on Core 0

## 🎮 Controls

| Control | Function | Details |
|---------|----------|---------|
| **DIAL1** (GPIO 4) | Volume | 0-100% |
| **DIAL2** (GPIO 33) | Unison | x1/x2/x3/x4 voices (chord modes) |
| **BOOT** short press | Waveform | SAW → SQR → TRI → SIN |
| **BOOT** long press | Mode | PROGRESSION → CHORD → NOTE |
| **OK** button (GPIO 13) | Waveform | SAW → SQR → TRI → SIN |
| **BACK** button (GPIO 16) | Mode | PROGRESSION → CHORD → NOTE |

## 🎹 Play Modes

### 1. PROGRESSION (default)
- Auto-plays chord progression
- Changes every 1.6 seconds (half note @ 75 BPM)
- Default: Ebmaj7 → Cm7 → Abmaj7 → Abmaj7

### 2. CHORD
- Holds single chord (Cm7)
- Adjust unison with DIAL2

### 3. NOTE
- Single 880Hz tone (A5)
- Clean sine wave

## 🔧 Hardware

### Required Components
- ESP32-WROOM-32
- MAX98357A I2S Audio Amplifier
- SSD1306 OLED Display (128x64, I2C)
- 2× 10kΩ Potentiometers
- Speaker (4-8Ω)

### Connections

See [WIRING.txt](WIRING.txt) for complete wiring diagram.

**Quick Reference:**
```
MAX98357A:  BCLK=GPIO25, LRC=GPIO26, DIN=GPIO22
OLED:       SDA=GPIO21, SCL=GPIO19
DIAL1:      Wiper=GPIO4 (Volume)
DIAL2:      Wiper=GPIO33 (Unison)
BOOT:       GPIO0 (built-in)
OK button:  GPIO13 → GND
BACK button: GPIO16 → GND
```

## 🚀 Getting Started

### Physical Hardware

1. **Wire the components** (see [WIRING.txt](WIRING.txt))

2. **Install libraries:**
   ```bash
   arduino-cli lib install "Adafruit GFX Library"
   arduino-cli lib install "Adafruit SSD1306"
   ```

3. **Upload to ESP32:**
   ```bash
   ./upload.sh
   ```

4. **Play!** Adjust volume, change waveforms, enjoy the synth!

### Wokwi Simulation (No Hardware Needed!)

Test the project in your browser without any hardware:

1. **Build firmware:**
   ```bash
   ./build-wokwi.sh
   ```

2. **Start simulator:**
   - Press `F1` in VS Code
   - Select: `Wokwi: Start Simulator`

3. **Interact:**
   - Drag potentiometer sliders
   - Click BOOT button
   - Watch the OLED display

**Note:** Audio output is not simulated in Wokwi (MAX98357A not supported), but all logic and display features work perfectly.

See [WOKWI_QUICKSTART.md](WOKWI_QUICKSTART.md) for quick start or [WOKWI_SETUP.md](WOKWI_SETUP.md) for detailed setup.

## 📁 Project Structure

```
chord-synth/
├── chord-synth.ino          # Main sketch
├── BootAnimation.h          # Startup animation
├── ChordLibrary.h           # Chord definitions
├── ChordPlayer.h            # Polyphonic chord player
├── Gauge.h                  # Animated gauge display
├── I2SDriver.h              # I2S audio driver
├── Oscillator.h             # Waveform generator
├── UnisonConfig.h           # Unison detuning config
├── upload.sh                # Upload to hardware
├── build-wokwi.sh           # Build for Wokwi
├── diagram.json             # Wokwi hardware layout
├── wokwi.toml               # Wokwi configuration
├── WIRING.txt               # Hardware wiring guide
├── WOKWI_QUICKSTART.md      # Quick Wokwi guide
└── WOKWI_SETUP.md           # Detailed Wokwi setup
```

## 🎨 Display Layout

```
┌────────────────────────────┐
│ Cm7      CHORD      V:75%  │  ← Chord/Freq, Mode, Volume
│ ▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░  │  ← Volume bar
│                            │
│        ╱╲╱╲╱╲╱╲╱╲         │  ← Waveform visualization
│ ───────────────────────── │  ← Center line
│       ╲╱╲╱╲╱╲╱╲╱           │
│                            │
│ 3 Notes                    │  ← Status info
└────────────────────────────┘
```

## 🎛️ Technical Details

- **Sample Rate:** 44.1 kHz
- **Audio Format:** 16-bit stereo
- **Buffer Size:** 512 frames
- **Waveform Table:** 2048 samples
- **Unison Detune:** ±0.5% to ±2.0% per voice
- **Display Update:** 10 FPS
- **Chord Duration:** 1.6 seconds (half note @ 75 BPM)

## 📚 Documentation

- [WIRING.txt](WIRING.txt) - Complete hardware wiring guide
- [WOKWI_QUICKSTART.md](WOKWI_QUICKSTART.md) - Quick Wokwi simulation guide
- [WOKWI_SETUP.md](WOKWI_SETUP.md) - Detailed Wokwi setup and troubleshooting
- [menu_system_readme.md](menu_system_readme.md) - Menu system documentation

## 🐛 Troubleshooting

### No Sound
- Check MAX98357A connections (BCLK, LRC, DIN)
- Verify speaker is connected
- Check volume (DIAL1)

### Display Not Working
- Verify I2C address (0x3C or 0x3D)
- Check SDA/SCL connections (GPIO 21/19)
- Try different OLED_ADDRESS in code

### Potentiometers Not Responding
- Verify 3.3V and GND connections
- Check wiper connections (GPIO 4, GPIO 33)
- Test with Serial monitor output

## 🔬 Serial Monitor

The synth outputs debug information:
```
Volume: 75%
Waveform: SQR
Mode: CHORD (Cm7)
Unison: x3
Progression: Ebmaj7
```

Open Serial Monitor @ 115200 baud to see real-time status.

## 🎓 Learning Resources

This project demonstrates:
- I2S audio output
- FreeRTOS multi-core tasks
- Mutex synchronization
- OLED graphics
- ADC input smoothing
- Waveform synthesis
- Polyphonic audio mixing

## 📝 License

Open source - feel free to modify and share!

## 🙏 Credits

Built with:
- Adafruit GFX Library
- Adafruit SSD1306 Library
- ESP32 Arduino Core

---

**Happy synthesizing! 🎹🎵**

