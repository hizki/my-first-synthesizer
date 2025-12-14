/*
 * Gauge.h - Reusable arc gauge display component
 * 
 * A configurable speedometer-style gauge with:
 * - Elliptic arc shape
 * - Customizable stop positions and labels
 * - Animated needle with arrow tip
 * - Support for evenly-spaced or custom angle stops
 */

#ifndef GAUGE_H
#define GAUGE_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/**
 * Gauge - A reusable arc gauge component for OLED displays
 * 
 * Features:
 * - Configurable number of stops via label count
 * - Optional custom angles or automatic even spacing
 * - Elliptic arc for space-efficient display
 * - Double-line arc for detailed appearance
 * - Arrow-tipped needle
 */
class Gauge {
public:
  /**
   * Constructor
   */
  Gauge() : 
    _display(nullptr),
    _centerX(0),
    _centerY(0),
    _radiusX(0),
    _radiusY(0),
    _numStops(0),
    _labels(nullptr),
    _angles(nullptr),
    _customAngles(nullptr),
    _currentAngle(0.0f),
    _isAnimating(false),
    _animationStartTime(0),
    _animationDuration(0),
    _animationHoldDuration(0),
    _previousAngle(0.0f),
    _targetAngle(0.0f) {
  }

  /**
   * Initialize the gauge with configuration
   * 
   * @param display Pointer to Adafruit_SSD1306 display object
   * @param centerX Center X coordinate
   * @param centerY Center Y coordinate
   * @param radiusX Horizontal radius (ellipse width)
   * @param radiusY Vertical radius (ellipse height)
   * @param labels Array of label strings for each stop
   * @param numLabels Number of labels (determines number of stops)
   * @param angles Optional array of custom angles (NULL for even spacing from 180° to 0°)
   */
  void init(Adafruit_SSD1306* display, int centerX, int centerY, 
            int radiusX, int radiusY, const char** labels, int numLabels,
            const float* angles = nullptr) {
    _display = display;
    _centerX = centerX;
    _centerY = centerY;
    _radiusX = radiusX;
    _radiusY = radiusY;
    _numStops = numLabels;
    _labels = labels;
    _customAngles = angles;
    
    // If custom angles provided, use them; otherwise generate evenly spaced angles
    if (_customAngles == nullptr && _numStops > 0) {
      // Allocate memory for evenly spaced angles (180° to 0°)
      _angles = new float[_numStops];
      float angleStep = 180.0f / (_numStops - 1);
      for (int i = 0; i < _numStops; i++) {
        _angles[i] = 180.0f - (angleStep * i);
      }
    } else {
      // Cast away const for internal storage pointer
      _angles = const_cast<float*>(_customAngles);
    }
    
    // Initialize needle to first stop position
    if (_angles != nullptr && _numStops > 0) {
      _currentAngle = _angles[0];
    }
  }

  /**
   * Destructor - clean up dynamically allocated angles if created
   */
  ~Gauge() {
    if (_customAngles == nullptr && _angles != nullptr) {
      delete[] _angles;
    }
  }

  /**
   * Set the current needle angle
   * 
   * @param angle Angle in degrees (0-360, where 180° is left, 0° is right)
   */
  void setAngle(float angle) {
    _currentAngle = angle;
  }

  /**
   * Start an animated transition to a target angle
   * 
   * @param targetAngle Target angle in degrees
   * @param animationDurationMs Animation duration in milliseconds (default: 450ms)
   * @param holdDurationMs Additional hold time after animation (default: 500ms)
   */
  void startAnimation(float targetAngle, unsigned long animationDurationMs = 450, 
                      unsigned long holdDurationMs = 500) {
    _previousAngle = _currentAngle;
    _targetAngle = targetAngle;
    _animationStartTime = millis();
    _animationDuration = animationDurationMs;
    _animationHoldDuration = holdDurationMs;
    _isAnimating = true;
  }

  /**
   * Check if animation is currently active
   * 
   * @return true if animating, false otherwise
   */
  bool isAnimating() const {
    if (!_isAnimating) return false;
    
    unsigned long elapsed = millis() - _animationStartTime;
    return elapsed < (_animationDuration + _animationHoldDuration);
  }

  /**
   * Update animation state (call this each frame)
   * Returns true if animation is still active
   */
  bool updateAnimation() {
    if (!_isAnimating) return false;
    
    unsigned long elapsed = millis() - _animationStartTime;
    
    // Check if animation is complete
    if (elapsed >= (_animationDuration + _animationHoldDuration)) {
      _isAnimating = false;
      _currentAngle = _targetAngle;
      return false;
    }
    
    // Calculate current angle based on animation progress
    if (elapsed < _animationDuration) {
      // Animating - interpolate between previous and target angles
      float progress = (float)elapsed / (float)_animationDuration;
      float angleDelta = getShortestAnglePath(_previousAngle, _targetAngle);
      _currentAngle = _previousAngle + (angleDelta * progress);
    } else {
      // Animation complete - use target angle (static hold period)
      _currentAngle = _targetAngle;
    }
    
    return true;
  }

  /**
   * Draw the complete gauge (arc, ticks, labels, needle)
   * Call this after setAngle() to render the gauge
   */
  void draw() {
    if (_display == nullptr || _angles == nullptr || _numStops == 0) {
      return;
    }
    
    drawArc();
    drawTicks();
    drawLabels();
    drawNeedle();
    drawPivot();
  }

  /**
   * Draw the gauge with animation and bottom label
   * 
   * @param bottomLabel Text to display at bottom center (or nullptr for no label)
   * @param labelSize Text size for bottom label (default: 2)
   */
  void drawWithLabel(const char* bottomLabel = nullptr, int labelSize = 2) {
    if (_display == nullptr) return;
    
    _display->clearDisplay();
    
    // Update animation if active
    updateAnimation();
    
    // Draw the gauge
    draw();
    
    // Draw bottom label if provided
    if (bottomLabel != nullptr) {
      _display->setTextSize(labelSize);
      _display->setTextColor(SSD1306_WHITE);
      int textWidth = strlen(bottomLabel) * (6 * labelSize);  // Approximate width
      int screenWidth = _display->width();
      int screenHeight = _display->height();
      _display->setCursor((screenWidth - textWidth) / 2, screenHeight - 16);
      _display->print(bottomLabel);
    }
    
    _display->display();
  }

private:
  // Display and geometry
  Adafruit_SSD1306* _display;
  int _centerX;
  int _centerY;
  int _radiusX;
  int _radiusY;
  
  // Stop configuration
  int _numStops;
  const char** _labels;
  float* _angles;              // Either points to _customAngles or dynamically allocated
  const float* _customAngles;  // User-provided custom angles (or nullptr)
  
  // Current state
  float _currentAngle;
  
  // Animation state
  bool _isAnimating;
  unsigned long _animationStartTime;
  unsigned long _animationDuration;
  unsigned long _animationHoldDuration;
  float _previousAngle;
  float _targetAngle;

  /**
   * Calculate shortest angle path between two angles
   * Handles wrapping for smooth animations
   * 
   * @param from Starting angle
   * @param to Target angle
   * @return Angle delta (can be negative for counterclockwise)
   */
  float getShortestAnglePath(float from, float to) {
    // Special case: SINE (0°) back to SAWTOOTH (180°) - go all the way back counterclockwise
    if (from == 0.0f && to == 180.0f) {
      return -180.0f;  // Go counterclockwise through the full sweep
    }
    
    float delta = to - from;
    
    // Normalize to [-180, 180] range to find shortest path
    while (delta > 180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;
    
    return delta;
  }

  /**
   * Draw the elliptic arc with double lines
   */
  void drawArc() {
    const float arcStartAngle = 180.0f;
    const float arcEndAngle = 0.0f;
    
    for (float angle = arcStartAngle; angle >= arcEndAngle; angle -= 1.5f) {
      float radians = angle * PI / 180.0f;
      
      // Outer arc line
      int x = _centerX + _radiusX * cos(radians);
      int y = _centerY - _radiusY * sin(radians);
      _display->drawPixel(x, y, SSD1306_WHITE);
      
      // Inner arc line (double-line effect)
      int x2 = _centerX + (_radiusX - 2) * cos(radians);
      int y2 = _centerY - (_radiusY - 2) * sin(radians);
      _display->drawPixel(x2, y2, SSD1306_WHITE);
    }
  }

  /**
   * Draw tick marks at each stop position
   */
  void drawTicks() {
    for (int i = 0; i < _numStops; i++) {
      float tickRadians = _angles[i] * PI / 180.0f;
      
      // Calculate tick endpoints (radial line from arc inward and outward)
      int tickX1 = _centerX + (_radiusX - 3) * cos(tickRadians);
      int tickY1 = _centerY - (_radiusY - 3) * sin(tickRadians);
      int tickX2 = _centerX + (_radiusX + 3) * cos(tickRadians);
      int tickY2 = _centerY - (_radiusY + 3) * sin(tickRadians);
      
      _display->drawLine(tickX1, tickY1, tickX2, tickY2, SSD1306_WHITE);
    }
  }

  /**
   * Draw text labels at each stop position
   */
  void drawLabels() {
    _display->setTextSize(1);
    _display->setTextColor(SSD1306_WHITE);
    
    for (int i = 0; i < _numStops; i++) {
      float labelRadians = _angles[i] * PI / 180.0f;
      
      // Position label outside the arc
      int labelX = _centerX + (_radiusX + 9) * cos(labelRadians);
      int labelY = _centerY - (_radiusY + 9) * sin(labelRadians);
      
      // Center the text on the tick position
      // Approximate width: each character is ~6 pixels wide in size 1
      int textWidth = strlen(_labels[i]) * 6;
      labelX -= textWidth / 2;
      labelY -= 4;  // Adjust vertically for better centering
      
      _display->setCursor(labelX, labelY);
      _display->print(_labels[i]);
    }
  }

  /**
   * Draw the needle pointer with arrow tip
   */
  void drawNeedle() {
    float needleRadians = _currentAngle * PI / 180.0f;
    
    // Calculate needle tip position
    int needleX = _centerX + (_radiusX - 8) * cos(needleRadians);
    int needleY = _centerY - (_radiusY - 8) * sin(needleRadians);
    
    // Draw thick needle line from center to tip
    _display->drawLine(_centerX, _centerY, needleX, needleY, SSD1306_WHITE);
    _display->drawLine(_centerX + 1, _centerY, needleX + 1, needleY, SSD1306_WHITE);
    _display->drawLine(_centerX, _centerY + 1, needleX, needleY + 1, SSD1306_WHITE);
    
    // Draw arrow tip at needle end
    // Calculate arrow wing positions
    float perpAngle1 = needleRadians + 2.5f;
    float perpAngle2 = needleRadians - 2.5f;
    
    int arrowWing1X = needleX - 4 * cos(perpAngle1);
    int arrowWing1Y = needleY + 4 * sin(perpAngle1);
    int arrowWing2X = needleX - 4 * cos(perpAngle2);
    int arrowWing2Y = needleY + 4 * sin(perpAngle2);
    
    // Draw arrow as a filled triangle
    _display->fillTriangle(needleX, needleY, arrowWing1X, arrowWing1Y, 
                          arrowWing2X, arrowWing2Y, SSD1306_WHITE);
  }

  /**
   * Draw the center pivot point
   */
  void drawPivot() {
    _display->fillCircle(_centerX, _centerY, 3, SSD1306_WHITE);
  }
};

#endif // GAUGE_H

