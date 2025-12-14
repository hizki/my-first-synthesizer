/*
 * BootAnimation.h - Dazzling boot animation for Chord Synth
 * 
 * A multi-phase animated boot sequence that showcases the synth's features
 * with waveforms, burst effects, pulsing text, and feature highlights.
 */

#ifndef BOOT_ANIMATION_H
#define BOOT_ANIMATION_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/**
 * BootAnimation - Animated boot sequence display
 * 
 * Features:
 * - Phase 1: Animated waveforms building up
 * - Phase 2: Burst effect with expanding circles
 * - Phase 3: Title reveal with pulsing effect
 */
class BootAnimation {
public:
  /**
   * Play the complete boot animation sequence
   * 
   * @param display Pointer to Adafruit_SSD1306 display object
   * @param screenWidth Screen width in pixels (default: 128)
   * @param screenHeight Screen height in pixels (default: 64)
   */
  static void play(Adafruit_SSD1306* display, int screenWidth = 128, int screenHeight = 64) {
    if (display == nullptr) return;
    
    const int centerY = screenHeight / 2;
    const int duration = 80;  // Duration for each frame
    
    // Phase 1: Animated waveforms building up (sawtooth, square, triangle, sine)
    for (int frame = 0; frame < 40; frame++) {
      display->clearDisplay();
      
      float progress = frame / 40.0f;
      float amplitude = 20.0f * progress;
      
      // Draw multiple overlapping waveforms with different phases
      for (int x = 0; x < screenWidth; x++) {
        float xNorm = x / (float)screenWidth;
        
        // Sawtooth wave
        float saw = (fmod(xNorm * 3 + progress * 0.5f, 1.0f) - 0.5f) * 2.0f;
        int y1 = centerY - (int)(amplitude * 0.5f * saw);
        
        // Sine wave
        float sine = sin((xNorm * 3 + progress * 0.3f) * TWO_PI);
        int y2 = centerY - (int)(amplitude * 0.7f * sine);
        
        // Draw pixels for both waves
        if (x > 0 && frame > 5) {
          display->drawPixel(x, y1, SSD1306_WHITE);
          display->drawPixel(x, y2, SSD1306_WHITE);
        }
      }
      
      display->display();
      delay(duration / 2);
    }
    
    // Phase 2: Burst effect with expanding circles
    for (int i = 0; i < 3; i++) {
      for (int r = 0; r < 50; r += 3) {
        display->clearDisplay();
        display->drawCircle(screenWidth / 2, centerY, r, SSD1306_WHITE);
        display->drawCircle(screenWidth / 2, centerY, r + 5, SSD1306_WHITE);
        display->display();
        delay(20);
      }
    }
    
    // Phase 3: Title reveal with pulsing effect
    for (int pulse = 0; pulse < 3; pulse++) {
      // Fade in
      for (int brightness = 0; brightness < 2; brightness++) {
        display->clearDisplay();
        
        // Draw "CHORD" with large text
        display->setTextSize(3);
        display->setTextColor(SSD1306_WHITE);
        display->setCursor(10, 5);
        display->println("CHORD");
        
        // Draw "SYNTH" below
        display->setCursor(10, 35);
        display->println("SYNTH");
        
        // Draw decorative waveform lines
        if (brightness == 1) {
          // Top line
          for (int x = 0; x < screenWidth; x++) {
            int y = 2 + (int)(2 * sin(x * 0.3f));
            display->drawPixel(x, y, SSD1306_WHITE);
          }
          
          // Bottom line
          for (int x = 0; x < screenWidth; x++) {
            int y = screenHeight - 3 + (int)(2 * sin(x * 0.3f + PI));
            display->drawPixel(x, y, SSD1306_WHITE);
          }
        }
        
        display->display();
        delay(200);
      }
      
      // Brief hold
      delay(150);
      
      // Fade out (only for first 2 pulses)
      if (pulse < 2) {
        display->clearDisplay();
        display->display();
        delay(100);
      }
    }
    
    // Final flourish - wipe effect
    for (int y = 0; y < screenHeight; y += 2) {
      display->drawFastHLine(0, y, screenWidth, SSD1306_BLACK);
      display->display();
      delay(5);
    }
    
    display->clearDisplay();
    display->display();
  }
};

#endif // BOOT_ANIMATION_H

