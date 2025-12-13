/*
 * ESP32 Audio Visualizer with I2S Output and Volume Control
 * 
 * Hardware:
 * - ESP32-WROOM-32
 * - MAX98357A I2S Audio Amplifier
 * - I2C OLED Display (SSD1306)
 * - Potentiometer (10kΩ) for volume control
 * 
 * Connections:
 * 
 * MAX98357A:
 *   BCLK -> GPIO 26
 *   LRC  -> GPIO 25
 *   DIN  -> GPIO 22
 *   VIN  -> 3.3V (or 5V)
 *   GND  -> GND
 * 
 * OLED Display:
 *   VCC -> 3.3V
 *   GND -> GND
 *   SCL -> GPIO 19
 *   SDA -> GPIO 21
 * 
 * Potentiometer:
 *   One side  -> 3.3V
 *   Wiper     -> GPIO 4 (D4)
 *   Other side -> GND
 * 
 * Libraries Required:
 * - Adafruit GFX Library
 * - Adafruit SSD1306
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "driver/i2s_std.h"
#include "driver/gpio.h"

// ========== OLED Display Configuration ==========
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1    // Reset pin (-1 if sharing ESP32 reset pin)
#define OLED_SDA      21    // I2C Data
#define OLED_SCL      19    // I2C Clock (custom pin to avoid conflict with I2S)
#define OLED_ADDRESS  0x3C  // I2C address (0x3C or 0x3D)

// Initialize OLED display using I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ========== I2S Pin Configuration ==========
// MAX98357A connections (from working example)
#define I2S_BCLK    26   // MAX98357A BCLK
#define I2S_LRCLK   25   // MAX98357A LRC / LRCLK / WS
#define I2S_DOUT    22   // MAX98357A DIN

// ========== Potentiometer Configuration ==========
#define POT_PIN     4    // Potentiometer for volume control (GPIO 4 / D4)

// ========== Button Configuration ==========
#define BOOT_BUTTON 0    // BOOT button (GPIO 0)

// ========== Oscillator Types ==========
enum OscillatorType {
  OSC_SINE = 0,
  OSC_TRIANGLE,
  OSC_SQUARE,
  OSC_SAWTOOTH,
  OSC_COUNT  // Total number of oscillator types
};

// ========== Audio Configuration ==========
#define I2S_PORT        I2S_NUM_0
#define SAMPLE_RATE     44100          // 44.1 kHz
#define TONE_FREQUENCY  880.0f        // A5, 880 Hz (higher frequency reduces speaker load)
#define TABLE_SIZE      256           // Sine table size
#define MAX_AMPLITUDE   14000         // Maximum amplitude for 16-bit audio (reduced to prevent clipping)

// I2S channel handle for new driver
i2s_chan_handle_t tx_handle = NULL;

// ========== FreeRTOS Task Handles ==========
TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t displayTaskHandle = NULL;
SemaphoreHandle_t volumeMutex = NULL;

// ========== Waveform Tables ==========
int16_t sineTable[TABLE_SIZE];
int16_t triangleTable[TABLE_SIZE];
int16_t squareTable[TABLE_SIZE];
int16_t sawtoothTable[TABLE_SIZE];

// ========== Shared Variables ==========
float currentAmplitude = 1.0f;        // Current amplitude multiplier (0.0 to 1.0)
int volumePercent = 100;              // Current volume percentage
volatile OscillatorType currentOscillator = OSC_SINE;  // Current oscillator type

// ========== Build Waveform Lookup Tables ==========
void buildWaveformTables() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float phase = (2.0f * PI * i) / TABLE_SIZE;
    
    // Sine wave
    sineTable[i] = (int16_t)(sinf(phase) * MAX_AMPLITUDE);
    
    // Triangle wave
    float triangleValue;
    if (i < TABLE_SIZE / 2) {
      triangleValue = (4.0f * i / TABLE_SIZE) - 1.0f;
    } else {
      triangleValue = 3.0f - (4.0f * i / TABLE_SIZE);
    }
    triangleTable[i] = (int16_t)(triangleValue * MAX_AMPLITUDE);
    
    // Square wave
    squareTable[i] = (i < TABLE_SIZE / 2) ? MAX_AMPLITUDE : -MAX_AMPLITUDE;
    
    // Sawtooth wave
    float sawtoothValue = (2.0f * i / TABLE_SIZE) - 1.0f;
    sawtoothTable[i] = (int16_t)(sawtoothValue * MAX_AMPLITUDE);
  }
}

// ========== Read Volume from Potentiometer ==========
void updateVolume() {
  // Read ADC value (0-4095 on ESP32)
  int adcValue = analogRead(POT_PIN);
  
  // Apply smoothing to reduce jitter
  static int smoothedADC = 2048;
  smoothedADC = (smoothedADC * 7 + adcValue) / 8;  // Simple low-pass filter
  
  // Convert to amplitude multiplier (0.0 to 1.0)
  currentAmplitude = smoothedADC / 4095.0f;
  
  // Convert to percentage for display
  volumePercent = (int)(currentAmplitude * 100);
  
  // Optional: Set minimum volume to avoid complete silence
  if (currentAmplitude < 0.05f) {
    currentAmplitude = 0.0f;
    volumePercent = 0;
  }
}

// ========== Button Handler with Debouncing ==========
void IRAM_ATTR onBootButtonPress() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  
  // Debounce: ignore if button pressed within 250ms
  if (interruptTime - lastInterruptTime > 250) {
    // Cycle to next oscillator type
    currentOscillator = (OscillatorType)((currentOscillator + 1) % OSC_COUNT);
    lastInterruptTime = interruptTime;
  }
}

// ========== Get Oscillator Name ==========
const char* getOscillatorName(OscillatorType osc) {
  switch (osc) {
    case OSC_SINE:     return "SINE";
    case OSC_TRIANGLE: return "TRI";
    case OSC_SQUARE:   return "SQR";
    case OSC_SAWTOOTH: return "SAW";
    default:           return "???";
  }
}

// ========== I2S Setup (New Driver API) ==========
void setupI2S() {
  // Create I2S channel configuration
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  chan_cfg.auto_clear = true;  // Auto clear DMA buffer
  chan_cfg.dma_desc_num = 16;   // Increased from 8 for better buffering
  chan_cfg.dma_frame_num = 256; // Increased from 64 for better buffering
  
  // Allocate new TX channel
  esp_err_t err = i2s_new_channel(&chan_cfg, &tx_handle, NULL);
  if (err != ESP_OK) {
    Serial.printf("Failed to create I2S channel: %d\n", err);
    return;
  }
  
  // Configure I2S standard mode (Philips standard)
  i2s_std_config_t std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = (gpio_num_t)I2S_BCLK,
      .ws = (gpio_num_t)I2S_LRCLK,
      .dout = (gpio_num_t)I2S_DOUT,
      .din = I2S_GPIO_UNUSED,
      .invert_flags = {
        .mclk_inv = false,
        .bclk_inv = false,
        .ws_inv = false,
      },
    },
  };
  
  // Initialize the channel
  err = i2s_channel_init_std_mode(tx_handle, &std_cfg);
  if (err != ESP_OK) {
    Serial.printf("Failed to initialize I2S standard mode: %d\n", err);
    return;
  }
  
  // Enable the channel
  err = i2s_channel_enable(tx_handle);
  if (err != ESP_OK) {
    Serial.printf("Failed to enable I2S channel: %d\n", err);
    return;
  }
  
  Serial.println("I2S initialized successfully (new driver).");
}

// ========== Display Setup ==========
void setupDisplay() {
  // Initialize I2C with custom pins (to avoid conflict with I2S GPIO22)
  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.print("I2C initialized on SDA=GPIO");
  Serial.print(OLED_SDA);
  Serial.print(", SCL=GPIO");
  Serial.println(OLED_SCL);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("ERROR: SSD1306 allocation failed!");
    Serial.println("Check I2C wiring and address (try 0x3C or 0x3D)");
    // Show error indicator but continue with audio
    return;
  }

  Serial.println("OLED display initialized successfully!");
  
  // Show boot splash
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("AUDIO");
  display.setCursor(5, 35);
  display.println("VISUALIZER");
  display.display();
  delay(1500);
  
  // Show frequency info
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 20);
  display.println("Playing:");
  display.setTextSize(2);
  display.setCursor(25, 35);
  display.print(TONE_FREQUENCY, 0);
  display.println(" Hz");
  display.display();
  delay(1500);
}

// ========== Setup ==========
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("ESP32 Audio Visualizer");
  Serial.println("========================================");
  Serial.println();

  // Initialize potentiometer pin
  pinMode(POT_PIN, INPUT);
  // Note: Not using analogSetAttenuation() to avoid conflict with I2S driver
  Serial.println("Potentiometer initialized on GPIO 4");

  // Initialize BOOT button with interrupt
  pinMode(BOOT_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BOOT_BUTTON), onBootButtonPress, FALLING);
  Serial.println("BOOT button initialized on GPIO 0");

  // Initialize display first
  setupDisplay();
  
  // Build waveform tables
  buildWaveformTables();
  
  // Initialize I2S audio
  setupI2S();
  
  // Read initial volume
  updateVolume();

  // Create mutex for volume synchronization
  volumeMutex = xSemaphoreCreateMutex();
  if (volumeMutex == NULL) {
    Serial.println("ERROR: Failed to create volume mutex!");
    while (1) delay(1000);
  }
  Serial.println("Mutex created successfully");

  // Create audio task on Core 1 (high priority)
  xTaskCreatePinnedToCore(
    audioTask,           // Task function
    "AudioTask",         // Task name
    4096,                // Stack size (bytes)
    NULL,                // Parameters
    2,                   // Priority (higher = more priority)
    &audioTaskHandle,    // Task handle
    1                    // Core 1
  );
  
  // Create display task on Core 0 (normal priority)
  xTaskCreatePinnedToCore(
    displayTask,         // Task function
    "DisplayTask",       // Task name
    4096,                // Stack size (bytes)
    NULL,                // Parameters
    1,                   // Priority (lower than audio)
    &displayTaskHandle,  // Task handle
    0                    // Core 0
  );

  Serial.print("Setup complete. Playing ");
  Serial.print(TONE_FREQUENCY, 0);
  Serial.println(" Hz sine wave.");
  Serial.print("Initial volume: ");
  Serial.print(volumePercent);
  Serial.println("%");
  Serial.println("Audio task running on Core 1, Display task on Core 0");
  Serial.println();
}

// ========== Audio Task (Core 1) ==========
void audioTask(void *parameter) {
  Serial.println("Audio task started on Core 1");
  
  // Audio generation variables
  const int frames = 512;  // Increased buffer size for smoother audio
  int16_t buffer[frames * 2];  // 2 samples per frame (L,R)
  static float phaseIndex = 0.0f;
  const float phaseIncrement = (TABLE_SIZE * TONE_FREQUENCY) / (float)SAMPLE_RATE;
  
  while (true) {
    // Update volume from potentiometer
    int adcValue = analogRead(POT_PIN);
    static int smoothedADC = 2048;
    smoothedADC = (smoothedADC * 7 + adcValue) / 8;
    
    // Update shared variables with mutex protection
    if (xSemaphoreTake(volumeMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      float newAmplitude = smoothedADC / 4095.0f;
      if (newAmplitude < 0.05f) {
        newAmplitude = 0.0f;
      }
      
      // Log significant changes
      if (abs(newAmplitude - currentAmplitude) > 0.05f) {
        currentAmplitude = newAmplitude;
        volumePercent = (int)(currentAmplitude * 100);
        Serial.print("Volume: ");
        Serial.print(volumePercent);
        Serial.println("%");
      } else {
        currentAmplitude = newAmplitude;
        volumePercent = (int)(currentAmplitude * 100);
      }
      
      xSemaphoreGive(volumeMutex);
    }
    
    // Generate audio buffer
    float localAmplitude;
    OscillatorType localOscillator;
    if (xSemaphoreTake(volumeMutex, pdMS_TO_TICKS(1)) == pdTRUE) {
      localAmplitude = currentAmplitude;
      localOscillator = currentOscillator;
      xSemaphoreGive(volumeMutex);
    } else {
      localAmplitude = 1.0f;  // Fallback
      localOscillator = OSC_SINE;
    }
    
    // Select the appropriate waveform table
    int16_t* waveTable;
    switch (localOscillator) {
      case OSC_SINE:     waveTable = sineTable; break;
      case OSC_TRIANGLE: waveTable = triangleTable; break;
      case OSC_SQUARE:   waveTable = squareTable; break;
      case OSC_SAWTOOTH: waveTable = sawtoothTable; break;
      default:           waveTable = sineTable; break;
    }
    
    for (int i = 0; i < frames; i++) {
      // Wrap phase index into table range
      if (phaseIndex >= TABLE_SIZE) {
        phaseIndex -= TABLE_SIZE;
      }
      
      int idx = (int)phaseIndex;
      int16_t sample = (int16_t)(waveTable[idx] * localAmplitude);
      
      // Stereo: copy same sample to L and R
      buffer[i * 2 + 0] = sample;  // Left
      buffer[i * 2 + 1] = sample;  // Right
      
      phaseIndex += phaseIncrement;
    }
    
    // Output audio through I2S
    size_t bytesWritten = 0;
    if (tx_handle != NULL) {
      i2s_channel_write(tx_handle, buffer, sizeof(buffer), &bytesWritten, portMAX_DELAY);
    }
    
    // Small yield to prevent watchdog issues
    taskYIELD();
  }
}

// ========== Display Task (Core 0) ==========
void displayTask(void *parameter) {
  Serial.println("Display task started on Core 0");
  
  while (true) {
    updateDisplay();
    vTaskDelay(pdMS_TO_TICKS(100));  // Update at 10 FPS
  }
}

// ========== Main Loop (mostly idle now) ==========
void loop() {
  // Tasks are running on their respective cores
  // Keep loop() minimal to avoid interfering with tasks
  delay(1000);
}

// ========== Update Display with Waveform ==========
void updateDisplay() {
  // Read shared variables with mutex protection
  float localAmplitude;
  int localVolumePercent;
  OscillatorType localOscillator;
  
  if (xSemaphoreTake(volumeMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    localAmplitude = currentAmplitude;
    localVolumePercent = volumePercent;
    localOscillator = currentOscillator;
    xSemaphoreGive(volumeMutex);
  } else {
    // Fallback if mutex unavailable
    localAmplitude = 1.0f;
    localVolumePercent = 100;
    localOscillator = OSC_SINE;
  }
  
  display.clearDisplay();
  
  // Draw frequency at top left
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(TONE_FREQUENCY, 0);
  display.print("Hz");
  
  // Draw oscillator type in the middle top
  display.setCursor(52, 0);
  display.print(getOscillatorName(localOscillator));
  
  // Draw volume percentage at top right
  display.setCursor(SCREEN_WIDTH - 36, 0);
  display.print("V:");
  display.print(localVolumePercent);
  display.print("%");
  
  // Draw volume bar indicator
  int barWidth = (localVolumePercent * (SCREEN_WIDTH - 4)) / 100;
  display.drawRect(0, 10, SCREEN_WIDTH, 6, SSD1306_WHITE);
  if (barWidth > 0) {
    display.fillRect(2, 12, barWidth, 2, SSD1306_WHITE);
  }
  
  // Draw center reference line for waveform
  int centerY = 42;  // Lower half of screen for waveform
  display.drawFastHLine(0, centerY, SCREEN_WIDTH, SSD1306_WHITE);
  
  // Draw waveform across the screen
  // Show 2 complete cycles for clear visualization
  static float animPhase = 0.0f;
  animPhase += 0.1f;  // Animate the wave
  if (animPhase >= TWO_PI) animPhase -= TWO_PI;
  
  // Scale waveform amplitude based on volume
  float waveAmplitude = 18.0f * localAmplitude;  // Max 18 pixels height
  
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    // Calculate waveform value based on oscillator type
    float angle = (TWO_PI * 2 * x / SCREEN_WIDTH) + animPhase;
    float waveValue = 0.0f;
    
    switch (localOscillator) {
      case OSC_SINE:
        waveValue = sin(angle);
        break;
      case OSC_TRIANGLE: {
        float normalizedPhase = fmod(angle, TWO_PI) / TWO_PI;
        if (normalizedPhase < 0.5f) {
          waveValue = (4.0f * normalizedPhase) - 1.0f;
        } else {
          waveValue = 3.0f - (4.0f * normalizedPhase);
        }
        break;
      }
      case OSC_SQUARE:
        waveValue = (fmod(angle, TWO_PI) < PI) ? 1.0f : -1.0f;
        break;
      case OSC_SAWTOOTH: {
        float normalizedPhase = fmod(angle, TWO_PI) / TWO_PI;
        waveValue = (2.0f * normalizedPhase) - 1.0f;
        break;
      }
    }
    
    int y = centerY - (int)(waveAmplitude * waveValue);
    
    // Draw the waveform
    if (x > 0) {
      float prevAngle = (TWO_PI * 2 * (x - 1) / SCREEN_WIDTH) + animPhase;
      float prevWaveValue = 0.0f;
      
      switch (localOscillator) {
        case OSC_SINE:
          prevWaveValue = sin(prevAngle);
          break;
        case OSC_TRIANGLE: {
          float normalizedPhase = fmod(prevAngle, TWO_PI) / TWO_PI;
          if (normalizedPhase < 0.5f) {
            prevWaveValue = (4.0f * normalizedPhase) - 1.0f;
          } else {
            prevWaveValue = 3.0f - (4.0f * normalizedPhase);
          }
          break;
        }
        case OSC_SQUARE:
          prevWaveValue = (fmod(prevAngle, TWO_PI) < PI) ? 1.0f : -1.0f;
          break;
        case OSC_SAWTOOTH: {
          float normalizedPhase = fmod(prevAngle, TWO_PI) / TWO_PI;
          prevWaveValue = (2.0f * normalizedPhase) - 1.0f;
          break;
        }
      }
      
      int prevY = centerY - (int)(waveAmplitude * prevWaveValue);
      display.drawLine(x - 1, prevY, x, y, SSD1306_WHITE);
    }
  }
  
  // Draw "A4" label at bottom left
  display.setTextSize(1);
  display.setCursor(0, SCREEN_HEIGHT - 8);
  display.print("A4 Note");
  
  // Show mute indicator if volume is zero
  if (localVolumePercent == 0) {
    display.setCursor(SCREEN_WIDTH - 24, SCREEN_HEIGHT - 8);
    display.print("MUTE");
  }
  
  display.display();
}

