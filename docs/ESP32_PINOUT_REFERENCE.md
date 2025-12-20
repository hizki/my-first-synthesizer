# ESP32 Development Board - Pin Reference

This document provides a comprehensive reference for the ESP32 development board GPIO pins based on the Circuits4You ESP32 DevKit v1.

## Quick Reference Images

- `esp32-pinout-table.png` - Detailed GPIO pin table
- `esp32-pinout-diagram.png` - Visual pinout diagram

## GPIO Pin Table

| GPIO | Input | Output | Notes |
|------|-------|--------|-------|
| 0 | Pulled up | OK | Outputs PWM signal at boot, must be LOW to enter flashing mode |
| 1 | TX pin | OK | Debug output at boot |
| 2 | OK | OK | Connected to on-board LED, must be left floating or LOW to enter flashing mode |
| 3 | OK | RX pin | HIGH at boot |
| 4 | OK | OK | |
| 5 | OK | OK | Outputs PWM signal at boot, strapping pin |
| 6 | ❌ | ❌ | Connected to the integrated SPI flash |
| 7 | ❌ | ❌ | Connected to the integrated SPI flash |
| 8 | ❌ | ❌ | Connected to the integrated SPI flash |
| 9 | ❌ | ❌ | Connected to the integrated SPI flash |
| 10 | ❌ | ❌ | Connected to the integrated SPI flash |
| 11 | ❌ | ❌ | Connected to the integrated SPI flash |
| 12 | OK | OK | Boot fails if pulled high, strapping pin |
| 13 | OK | OK | |
| 14 | OK | OK | Outputs PWM signal at boot |
| 15 | OK | OK | Outputs PWM signal at boot, strapping pin |
| 16 | OK | OK | |
| 17 | OK | OK | |
| 18 | OK | OK | |
| 19 | OK | OK | |
| 21 | OK | OK | |
| 22 | OK | OK | |
| 23 | OK | OK | |
| 25 | OK | OK | |
| 26 | OK | OK | |
| 27 | OK | OK | |
| 32 | OK | OK | |
| 33 | OK | OK | |
| 34 | OK | - | Input only |
| 35 | OK | - | Input only |
| 36 | OK | - | Input only |
| 39 | OK | - | Input only |

## Pin Categories

### ❌ Do Not Use
**GPIOs 6-11** are connected to the integrated SPI flash memory. Using these pins will cause the ESP32 to malfunction.

### ⚠️ Strapping Pins (Use with Caution)
These pins have special functions during boot and should be used carefully:

- **GPIO 0**: Must be LOW to enter flashing mode
- **GPIO 2**: Must be floating or LOW to enter flashing mode (also connected to on-board LED)
- **GPIO 5**: Strapping pin, outputs PWM at boot
- **GPIO 12**: Boot fails if pulled HIGH
- **GPIO 15**: Strapping pin, outputs PWM at boot

### 🔌 Input-Only Pins
**GPIOs 34, 35, 36, 39** can only be used as inputs. They do not have internal pull-up/pull-down resistors.

### 📡 Communication Pins
- **GPIO 1**: TX pin (UART0) - used for debug output at boot
- **GPIO 3**: RX pin (UART0) - HIGH at boot

### 💡 On-Board LED
**GPIO 2** is connected to the on-board LED on most ESP32 development boards.

### ⚡ PWM Pins at Boot
These pins output PWM signals during boot:
- GPIO 0
- GPIO 5
- GPIO 14
- GPIO 15

## Peripheral Mapping

### ADC (Analog to Digital Converter)
- **ADC1**: GPIOs 32-39 (8 channels)
- **ADC2**: GPIOs 0, 2, 4, 12-15, 25-27 (10 channels)
  - ⚠️ ADC2 cannot be used when WiFi is active

### DAC (Digital to Analog Converter)
- **DAC1**: GPIO 25
- **DAC2**: GPIO 26

### Touch Sensors
- T0: GPIO 4
- T1: GPIO 0
- T2: GPIO 2
- T3: GPIO 15
- T4: GPIO 13
- T5: GPIO 12
- T6: GPIO 14
- T7: GPIO 27
- T8: GPIO 33
- T9: GPIO 32

### SPI
- **VSPI (Default)**
  - MOSI: GPIO 23
  - MISO: GPIO 19
  - CLK: GPIO 18
  - CS: GPIO 5

- **HSPI**
  - MOSI: GPIO 13
  - MISO: GPIO 12
  - CLK: GPIO 14
  - CS: GPIO 15

### I2C
Can be configured on any GPIO, but commonly used:
- **SDA**: GPIO 21
- **SCL**: GPIO 22

### UART
- **UART0** (USB/Serial)
  - TX: GPIO 1
  - RX: GPIO 3

- **UART1** (Flexible)
  - TX: GPIO 10 (usually connected to flash)
  - RX: GPIO 9 (usually connected to flash)

- **UART2** (Flexible)
  - TX: GPIO 17
  - RX: GPIO 16

## Best Practices

### Safe GPIOs for General Use
These pins are safe to use for most applications without special considerations:
- GPIO 4, 13, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33

### Pins to Avoid for Critical Applications
- Avoid strapping pins (0, 2, 5, 12, 15) for critical startup logic
- Never use GPIOs 6-11 (flash memory)
- Be careful with GPIO 1 and 3 if using Serial debugging

### Power Considerations
- Input voltage: 3.3V logic level
- Maximum current per GPIO: 12mA (recommended), 40mA (absolute maximum)
- Total current for all GPIOs: 200mA maximum

## References

- Board: Circuits4You ESP32 DevKit v1
- Chip: ESP-WROOM-32
- Images: See `docs/` folder for visual pinout references

## Additional Resources

- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)

