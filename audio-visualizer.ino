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
 * BOOT Button:
 *   GPIO 0 (built-in on ESP32-WROOM-32)
 *   Press to toggle between NOTE mode (single tone) and CHORD mode (Cm7 chord)
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

// NOTE: Oscillator.h is preserved for future multi-waveform feature
// Currently using ChordPlayer for chord mode implementation
// #include "Oscillator.h"  // PRESERVED - will be restored when adding waveform selection
#include "ChordLibrary.h"
#include "ChordPlayer.h"

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

// ========== Play Mode ==========
enum PlayMode {
  MODE_SINGLE_NOTE,
  MODE_CHORD,
  MODE_PROGRESSION
};

// ========== Audio Configuration ==========
#define I2S_PORT        I2S_NUM_0
#define SAMPLE_RATE     44100          // 44.1 kHz
#define TONE_FREQUENCY  880.0f        // A5, 880 Hz (higher frequency reduces speaker load)

// I2S channel handle for new driver
i2s_chan_handle_t tx_handle = NULL;

// ========== FreeRTOS Task Handles ==========
TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t displayTaskHandle = NULL;
SemaphoreHandle_t volumeMutex = NULL;

// ========== Audio Generators ==========
// Oscillator oscillator;  // PRESERVED - for future multi-waveform feature
ChordPlayer chordPlayer;

// ========== Shared Variables ==========
float currentAmplitude = 1.0f;        // Current amplitude multiplier (0.0 to 1.0)
int volumePercent = 100;              // Current volume percentage
volatile PlayMode currentMode = MODE_SINGLE_NOTE;  // Current play mode

// ========== Chord Progression Timing ==========
unsigned long lastChordChangeTime = 0;
const unsigned long CHORD_DURATION_MS = 1600;  // 1.6 seconds (half note at 75 BPM)
volatile int currentChordIndex = 0;
const Chord* const* currentProgression = ChordLib::JAZZ_PROGRESSION_1;
int currentProgressionLength = ChordLib::JAZZ_PROGRESSION_1_LENGTH;

// ========== Single Note Generation (for NOTE mode) ==========
const int SINE_TABLE_SIZE = 256;
int16_t sineTableNote[SINE_TABLE_SIZE];
const int16_t MAX_AMPLITUDE_SINGLE = 14000;


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
    // Cycle through 3 modes: NOTE -> CHORD -> PROGRESSION -> NOTE...
    if (currentMode == MODE_SINGLE_NOTE) {
      currentMode = MODE_CHORD;
      chordPlayer.setWaveform(WAVE_SINE);  // Use sine for static chord
      chordPlayer.reset();
      chordPlayer.setChord(&ChordLib::CM7);  // Set to Cm7
      Serial.println("Mode: CHORD (Cm7) [Sine]");
    } else if (currentMode == MODE_CHORD) {
      currentMode = MODE_PROGRESSION;
      currentChordIndex = 0;  // Start with first chord
      chordPlayer.setWaveform(WAVE_SAWTOOTH);  // Use sawtooth for progression
      chordPlayer.setChordFromProgression(0, currentProgression, currentProgressionLength);
      chordPlayer.reset();
      lastChordChangeTime = millis();  // Initialize timing
      Serial.println("Mode: PROGRESSION (Ebmaj7 -> Cm7 -> Abmaj7 -> Abmaj7) [Sawtooth]");
    } else {
      currentMode = MODE_SINGLE_NOTE;
      Serial.println("Mode: NOTE (880Hz)");
    }
    lastInterruptTime = interruptTime;
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
  
  // Build sine table for single note mode
  for (int i = 0; i < SINE_TABLE_SIZE; i++) {
    float phase = (2.0f * PI * i) / SINE_TABLE_SIZE;
    sineTableNote[i] = (int16_t)(sinf(phase) * MAX_AMPLITUDE_SINGLE);
  }
  Serial.println("Single note sine table built");
  
  // Initialize chord player
  chordPlayer.buildTable();
  chordPlayer.init(SAMPLE_RATE);
  Serial.println("Chord player initialized");
  
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

  Serial.println("Setup complete!");
  Serial.println("Mode: NOTE (single tone)");
  Serial.print("Frequency: ");
  Serial.print(TONE_FREQUENCY, 0);
  Serial.println(" Hz");
  Serial.print("Initial volume: ");
  Serial.print(volumePercent);
  Serial.println("%");
  Serial.println("Audio task running on Core 1, Display task on Core 0");
  Serial.println();
  Serial.println("Press BOOT button to cycle modes:");
  Serial.println("  NOTE -> CHORD -> PROGRESSION -> NOTE...");
  Serial.println("Progression: Ebmaj7 -> Cm7 -> Abmaj7 -> Abmaj7 @ 75 BPM");
  Serial.println();
}

// ========== Audio Task (Core 1) ==========
void audioTask(void *parameter) {
  Serial.println("Audio task started on Core 1");
  
  // Audio generation variables
  const int frames = 512;  // Increased buffer size for smoother audio
  int16_t buffer[frames * 2];  // 2 samples per frame (L,R)
  static float phaseIndex = 0.0f;
  const float phaseIncrement = (SINE_TABLE_SIZE * TONE_FREQUENCY) / (float)SAMPLE_RATE;
  
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
    
    // Handle chord progression timing (only in PROGRESSION mode)
    if (currentMode == MODE_PROGRESSION) {
      unsigned long currentTime = millis();
      if (currentTime - lastChordChangeTime >= CHORD_DURATION_MS) {
        // Time to switch to next chord
        currentChordIndex = (currentChordIndex + 1) % currentProgressionLength;
        chordPlayer.setChordFromProgression(currentChordIndex, currentProgression, currentProgressionLength);
        lastChordChangeTime = currentTime;
        
        // Log chord changes
        Serial.print("Progression: ");
        Serial.println(chordPlayer.getChordName());
      }
    }
    
    // Generate audio buffer
    float localAmplitude;
    PlayMode localMode;
    if (xSemaphoreTake(volumeMutex, pdMS_TO_TICKS(1)) == pdTRUE) {
      localAmplitude = currentAmplitude;
      localMode = currentMode;
      xSemaphoreGive(volumeMutex);
    } else {
      localAmplitude = 1.0f;  // Fallback
      localMode = MODE_SINGLE_NOTE;
    }
    
    // Generate samples based on current mode
    if (localMode == MODE_SINGLE_NOTE) {
      // Single note mode - 880Hz sine wave
      for (int i = 0; i < frames; i++) {
        // Wrap phase index into table range
        if (phaseIndex >= SINE_TABLE_SIZE) {
          phaseIndex -= SINE_TABLE_SIZE;
        }
        
        int idx = (int)phaseIndex;
        int16_t sample = (int16_t)(sineTableNote[idx] * localAmplitude);
        
        // Stereo: copy same sample to L and R
        buffer[i * 2 + 0] = sample;  // Left
        buffer[i * 2 + 1] = sample;  // Right
        
        phaseIndex += phaseIncrement;
      }
    } else if (localMode == MODE_CHORD || localMode == MODE_PROGRESSION) {
      // Chord modes - use ChordPlayer (handles both static and progression)
      for (int i = 0; i < frames; i++) {
        int16_t sample = (int16_t)(chordPlayer.getNextSample() * localAmplitude);
        
        // Stereo: copy same sample to L and R
        buffer[i * 2 + 0] = sample;  // Left
        buffer[i * 2 + 1] = sample;  // Right
      }
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
  PlayMode localMode;
  int localChordIndex;
  
  if (xSemaphoreTake(volumeMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    localAmplitude = currentAmplitude;
    localVolumePercent = volumePercent;
    localMode = currentMode;
    localChordIndex = currentChordIndex;
    xSemaphoreGive(volumeMutex);
  } else {
    // Fallback if mutex unavailable
    localAmplitude = 1.0f;
    localVolumePercent = 100;
    localMode = MODE_SINGLE_NOTE;
    localChordIndex = 0;
  }
  
  display.clearDisplay();
  
  // Draw frequency/chord name at top left
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  if (localMode == MODE_SINGLE_NOTE) {
    display.print(TONE_FREQUENCY, 0);
    display.print("Hz");
  } else {
    display.print(chordPlayer.getChordName());
  }
  
  // Draw mode in the middle top
  display.setCursor(52, 0);
  if (localMode == MODE_SINGLE_NOTE) {
    display.print("NOTE");
  } else if (localMode == MODE_CHORD) {
    display.print("CHORD");
  } else {
    display.print("PROG");
  }
  
  // Draw progression position if in progression mode
  if (localMode == MODE_PROGRESSION) {
    display.setCursor(SCREEN_WIDTH - 24, 0);
    display.print(localChordIndex + 1);
    display.print("/");
    display.print(currentProgressionLength);
  }
  
  // Draw volume percentage at top right (skip if in progression mode)
  if (localMode != MODE_PROGRESSION) {
    display.setCursor(SCREEN_WIDTH - 36, 0);
    display.print("V:");
    display.print(localVolumePercent);
    display.print("%");
  }
  
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
    float waveValue;
    
    if (localMode == MODE_SINGLE_NOTE) {
      // Single sine wave for note mode
      float angle = (TWO_PI * 2 * x / SCREEN_WIDTH) + animPhase;
      waveValue = sin(angle);
    } else {
      // Combined waveform for chord modes
      float time = (x / (float)SCREEN_WIDTH) * 0.1f + animPhase * 0.01f;
      waveValue = chordPlayer.getDisplayValue(time);
    }
    
    int y = centerY - (int)(waveAmplitude * waveValue);
    
    // Draw the waveform
    if (x > 0) {
      float prevWaveValue;
      if (localMode == MODE_SINGLE_NOTE) {
        float prevAngle = (TWO_PI * 2 * (x - 1) / SCREEN_WIDTH) + animPhase;
        prevWaveValue = sin(prevAngle);
      } else {
        float prevTime = ((x - 1) / (float)SCREEN_WIDTH) * 0.1f + animPhase * 0.01f;
        prevWaveValue = chordPlayer.getDisplayValue(prevTime);
      }
      
      int prevY = centerY - (int)(waveAmplitude * prevWaveValue);
      display.drawLine(x - 1, prevY, x, y, SSD1306_WHITE);
    }
  }
  
  // Draw mode label at bottom left
  display.setTextSize(1);
  display.setCursor(0, SCREEN_HEIGHT - 8);
  if (localMode == MODE_SINGLE_NOTE) {
    display.print("A5 Note");
  } else if (localMode == MODE_PROGRESSION) {
    display.print("75 BPM");
  } else {
    display.print("3 Notes");
  }
  
  // Show mute indicator if volume is zero
  if (localVolumePercent == 0) {
    display.setCursor(SCREEN_WIDTH - 24, SCREEN_HEIGHT - 8);
    display.print("MUTE");
  }
  
  display.display();
}

