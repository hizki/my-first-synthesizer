/*
 * I2SDriver.h - I2S audio output driver for ESP32
 * 
 * Encapsulates I2S configuration and output for MAX98357A amplifier
 * Uses the new ESP-IDF I2S driver API (i2s_std)
 */

#ifndef I2S_DRIVER_H
#define I2S_DRIVER_H

#include <Arduino.h>
#include "driver/i2s_std.h"
#include "driver/gpio.h"

/**
 * I2SDriver - Manages I2S audio output
 * 
 * Features:
 * - Stereo 16-bit audio output
 * - Configurable sample rate
 * - Uses new ESP-IDF I2S standard mode driver
 * - DMA buffering for smooth playback
 */
class I2SDriver {
public:
  /**
   * Constructor
   */
  I2SDriver() : 
    _txHandle(nullptr),
    _sampleRate(44100),
    _bclkPin(26),
    _lrclkPin(25),
    _doutPin(22),
    _isInitialized(false) {
  }

  /**
   * Initialize I2S driver with pin configuration
   * 
   * @param sampleRate Sample rate in Hz (default: 44100)
   * @param bclkPin Bit clock pin (default: 26)
   * @param lrclkPin Left/right clock pin (default: 25)
   * @param doutPin Data out pin (default: 22)
   * @return true if initialization successful, false otherwise
   */
  bool init(uint32_t sampleRate = 44100, int bclkPin = 26, int lrclkPin = 25, int doutPin = 22) {
    _sampleRate = sampleRate;
    _bclkPin = bclkPin;
    _lrclkPin = lrclkPin;
    _doutPin = doutPin;

    // Create I2S channel configuration
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;  // Auto clear DMA buffer
    chan_cfg.dma_desc_num = 16;   // Increased from 8 for better buffering
    chan_cfg.dma_frame_num = 256; // Increased from 64 for better buffering
    
    // Allocate new TX channel
    esp_err_t err = i2s_new_channel(&chan_cfg, &_txHandle, NULL);
    if (err != ESP_OK) {
      Serial.printf("I2S: Failed to create channel: %d\n", err);
      return false;
    }
    
    // Configure I2S standard mode (Philips standard)
    i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sampleRate),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
      .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = (gpio_num_t)_bclkPin,
        .ws = (gpio_num_t)_lrclkPin,
        .dout = (gpio_num_t)_doutPin,
        .din = I2S_GPIO_UNUSED,
        .invert_flags = {
          .mclk_inv = false,
          .bclk_inv = false,
          .ws_inv = false,
        },
      },
    };
    
    // Initialize the channel
    err = i2s_channel_init_std_mode(_txHandle, &std_cfg);
    if (err != ESP_OK) {
      Serial.printf("I2S: Failed to initialize standard mode: %d\n", err);
      return false;
    }
    
    // Enable the channel
    err = i2s_channel_enable(_txHandle);
    if (err != ESP_OK) {
      Serial.printf("I2S: Failed to enable channel: %d\n", err);
      return false;
    }
    
    _isInitialized = true;
    Serial.println("I2S initialized successfully (new driver).");
    Serial.printf("  Sample rate: %d Hz\n", _sampleRate);
    Serial.printf("  BCLK: GPIO %d, LRCLK: GPIO %d, DOUT: GPIO %d\n", _bclkPin, _lrclkPin, _doutPin);
    
    return true;
  }

  /**
   * Write audio samples to I2S output
   * 
   * @param buffer Pointer to audio buffer (interleaved stereo 16-bit samples)
   * @param bufferSize Size of buffer in bytes
   * @param bytesWritten Pointer to store actual bytes written (optional)
   * @return true if write successful, false otherwise
   */
  bool write(const void* buffer, size_t bufferSize, size_t* bytesWritten = nullptr) {
    if (!_isInitialized || _txHandle == nullptr) {
      return false;
    }
    
    size_t written = 0;
    esp_err_t err = i2s_channel_write(_txHandle, buffer, bufferSize, &written, portMAX_DELAY);
    
    if (bytesWritten != nullptr) {
      *bytesWritten = written;
    }
    
    return (err == ESP_OK);
  }

  /**
   * Check if driver is initialized
   * 
   * @return true if initialized, false otherwise
   */
  bool isInitialized() const {
    return _isInitialized;
  }

  /**
   * Get sample rate
   * 
   * @return Sample rate in Hz
   */
  uint32_t getSampleRate() const {
    return _sampleRate;
  }

  /**
   * Destructor - clean up I2S channel
   */
  ~I2SDriver() {
    if (_txHandle != nullptr) {
      i2s_channel_disable(_txHandle);
      i2s_del_channel(_txHandle);
    }
  }

private:
  i2s_chan_handle_t _txHandle;
  uint32_t _sampleRate;
  int _bclkPin;
  int _lrclkPin;
  int _doutPin;
  bool _isInitialized;
};

#endif // I2S_DRIVER_H

