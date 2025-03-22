#include "Animations.h"



void Animations::colorWipe2(unsigned long wipeTime) {
  static unsigned long wipeStartTime = 0;
  static int left = 0;
  static int right = _cLength - 1;
  static bool wipingForward = true;
  static bool wipeOff = false;
  static bool animationComplete = false;
  static uint32_t color = Colors[0];
  static uint32_t color1 = Colors[0];
  
  unsigned long currentMillis = millis();
  
  // Initialize wipe start time if this is the first call
  if (wipeStartTime == 0) {
    wipeStartTime = currentMillis;
  }
  
  // Determine the interval between updates
  unsigned long elapsedTime = currentMillis - wipeStartTime;
  unsigned long totalSteps = _cLength; // Total steps from end to end
  unsigned long wipeInterval = wipeTime / totalSteps;
  
  // Perform wiping effect if enough time has passed
  if (elapsedTime >= wipeInterval) {
    // Clear LEDs at previous positions
    if (wipingForward || wipeOff) {
      for (int j = 0; j < _sLength; j++) {
        _strip[j]->setPixelColor(left, 0);
        _strip[j]->setPixelColor(right, 0);
      }
    }

    if (wipingForward) {
      // Set color at current positions
      for (int j = 0; j < _sLength; j++) {
        _strip[j]->setPixelColor(left, color);
        _strip[j]->setPixelColor(right, color1);
      }
      
      left++;
      right--;
      
      // Reverse direction if collision is detected
      if (left > right) {
        // Stop forward wipe and start wipe off
        wipingForward = false;
        wipeOff = true;
        left--;
        right++;
      }
    } else if (wipeOff) {
      // Continue wiping effect in forward direction (to turn off LEDs)
      for (int j = 0; j < _sLength; j++) {
        _strip[j]->setPixelColor(left, 0);
        _strip[j]->setPixelColor(right, 0);
      }

      left++;
      right--;

      // End wipe off animation if reaching the end
      if (left >= _cLength) {
        animationComplete = true; // Mark animation as complete
      }
    }
    
    // Update the LEDs
    for (int j = 0; j < _sLength; j++) {
      _strip[j]->show();
    }
    
    // Reset start time for the next frame
    wipeStartTime = currentMillis;
  }
  
  // Reset state if animation is complete
  if (animationComplete) {
    // Turn off all LEDs
    for (int j = 0; j < _sLength; j++) {
      for (int i = 0; i < _cLength; i++) {
        _strip[j]->setPixelColor(i, 0);
      }
      _strip[j]->show();
    }

    color1 = color;
    color = getRandomColor();
    
    // Reset state
    left = 0;
    right = _cLength - 1;
    wipingForward = true;
    wipeOff = false;
    animationComplete = false;
  }
}


// Clear a specific strip
void Animations::clearStrip(size_t stripIndex) {
    if (stripIndex < _sLength) {
        _strip[stripIndex]->clear();
        _strip[stripIndex]->show();
    }
}

// Twinkle effect on all strips
void Animations::drawTwinkle(unsigned long TWINKLE_DELAY) {
    unsigned long currentMillis = millis();

    // Check if it's time to update twinkle (non-blocking)
    if (currentMillis - previousMillisTwinkle1 >= TWINKLE_DELAY) {
        previousMillisTwinkle1 = currentMillis;  // Save the last update time

        // Clear each strip before drawing the next twinkle
        for (size_t i = 0; i < _sLength; i++) {
            Animations::clearStrip(i);

            // Set random pixels to random colors
            for (int j = 0; j < _cLength / 4; j++) {
                int index = random(_cLength);            // Choose a random LED
                uint32_t color = getRandomColor(); // Choose a random color
                _strip[i]->setPixelColor(index, color);      // Set pixel to the chosen color
            }
            _strip[i]->show();  // Update the strip
        }
    }
}


void Animations::DrawMarquee() {
    static byte j = 0;
    j += 4;
    byte k = j;

    for (size_t s = 0; s < _sLength; s++) {
        Adafruit_NeoPixel& strip = *_strip[s];
        for (int i = 0; i < strip.numPixels(); i++) {
            uint32_t color = strip.ColorHSV(k * 256);  // Convert hue to 16-bit color
            strip.setPixelColor(i, color);
            k += 8;
        }

        static int scroll = 0;
        scroll++;

        for (int i = scroll % 5; i < strip.numPixels() - 1; i += 5) {
            strip.setPixelColor(i, strip.Color(0, 0, 0));  // Set to black
        }

        strip.show();  // Update the LED strip
    }
}

void Animations::DrawMarqueeMirrored() {
    static byte j = 0;
    j += 4;
    byte k = j;

    for (size_t s = 0; s < _sLength; s++) {
        Adafruit_NeoPixel& strip = *_strip[s];
        for (int i = 0; i < (strip.numPixels() + 1) / 2; i++) {
            uint32_t color = strip.ColorHSV(k * 256);  // Convert hue to 16-bit color
            strip.setPixelColor(i, color);
            strip.setPixelColor(strip.numPixels() - 1 - i, color);
            k += 8;
        }

        static int scroll = 0;
        scroll++;

        for (int i = scroll % 5; i < strip.numPixels() / 2; i += 5) {
            strip.setPixelColor(i, strip.Color(0, 0, 0));  // Set to black
            strip.setPixelColor(strip.numPixels() - 1 - i, strip.Color(0, 0, 0));
        }

        strip.show();  // Update the LED strip
    }
}

void Animations::DrawMarqueeDrawMarqueeMirrored(unsigned long interval) {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillisMarquee >= interval) {
        previousMillisMarquee = currentMillis;
        DrawMarquee();
    }

    if (currentMillis - previousMillisMarqueeMirrored >= interval) {
        previousMillisMarqueeMirrored = currentMillis;
        DrawMarqueeMirrored();
    }
}

void Animations::CometEffect(int cometSize, int deltaHue, unsigned long interval_) {
    unsigned long currentMillis1 = millis();
    if (currentMillis1 - previousMillis1 >= interval_) {
        previousMillis1 = currentMillis1;

        hue += deltaHue;

        // Update position
        iPos += iDirection;

        // Check if the comet has reached the end or beginning of the strip
        if (iPos >= _strip[0]->numPixels() - cometSize || iPos < 0) {
            iDirection *= -1;  // Reverse direction

            // Correct iPos to stay within bounds after reversing direction
            if (iPos >= _strip[0]->numPixels() - cometSize) {
                iPos = _strip[0]->numPixels() - cometSize;
            }
            if (iPos < 0) {
                iPos = 0;
            }
        }

        // Update each strip with the comet effect
        for (size_t i = 0; i < _sLength; i++) {
            Adafruit_NeoPixel* strip = _strip[i];

            // Clear previous comet effect
            for (int j = 0; j < strip->numPixels(); j++) {
                strip->setPixelColor(j, strip->Color(0, 0, 0));
            }

            // Set the comet's color
            for (int j = 0; j < cometSize; j++) {
                if (iPos + j < strip->numPixels()) {
                    strip->setPixelColor(iPos + j, strip->ColorHSV(hue * 65536L / 256));
                }
            }

            // Randomly fade the LEDs
            for (int k = 0; k < strip->numPixels(); k++) {
                if (random(10) > 5) {
                    uint32_t currentColor = strip->getPixelColor(k);
                    uint8_t r = (currentColor >> 16) & 0xFF;
                    uint8_t g = (currentColor >> 8) & 0xFF;
                    uint8_t b = currentColor & 0xFF;

                    // Fade the colors
                    r = (r > _fadeRate) ? r - _fadeRate : 0;
                    g = (g > _fadeRate) ? g - _fadeRate : 0;
                    b = (b > _fadeRate) ? b - _fadeRate : 0;

                    strip->setPixelColor(k, strip->Color(r, b, g));
                }
            }

            strip->show();
        }
    }
}



void Animations::colorWipe(unsigned long wipeTime){
  static unsigned long wipeStartTime = 0;
  static int left = 0;
  static int right = _cLength - 1;
  static bool wipingForward = true;
  static bool wipingOff = false;
  static bool animationComplete = false;
  static uint32_t color = Colors[0];

  unsigned long currentMillis = millis();

  // Initialize wipe start time if this is the first call
  if (wipeStartTime == 0) {
    wipeStartTime = currentMillis;
  }

  // Determine the interval between updates
  unsigned long totalSteps = _cLength; // Total steps from end to end
  unsigned long wipeInterval = wipeTime / totalSteps;

  // Perform wiping effect if enough time has passed
  if (currentMillis - wipeStartTime >= wipeInterval) {
    if (wipingForward) {
      // Set color at current positions
      for (int j = 0; j < _sLength; j++) {
        _strip[j]->setPixelColor(left, color);
        _strip[j]->setPixelColor(right, color);
      }
    } else if (wipingOff) {
      // Clear color at current positions
      for (int j = 0; j < _sLength; j++) {
        _strip[j]->setPixelColor(left, 0);
        _strip[j]->setPixelColor(right, 0);
      }
    }

    // Update the LEDs
    for (int j = 0; j < _sLength; j++) {
      _strip[j]->show();
    }

    // Move indices towards the center
    if (wipingForward) {
      left++;
      right--;
      // Reverse direction if collision is detected
      if (left > right) {
        wipingForward = false; // Switch direction to wiping off
        wipingOff = true;
        left = 0;
        right = _cLength - 1;
        wipeStartTime = currentMillis; // Restart timer for wipe off
      }
    } else if (wipingOff) {
      left++;
      right--;
      // End animation if reaching the original start position
      if (left > right) {
        animationComplete = true; // Mark animation as complete
      }
    }

    // Reset start time for the next frame
    wipeStartTime = currentMillis;
  }

  // Reset state if animation is complete
  if (animationComplete) {
    // Turn off all LEDs and reset state
    for (int j = 0; j < _sLength; j++) {
      for (int i = 0; i < _cLength; i++) {
        _strip[j]->setPixelColor(i, 0);
      }
      _strip[j]->show();
      color = getRandomColor();
    }

    left = 0;
    right = _cLength - 1;
    wipingForward = true;
    wipingOff = false;
    animationComplete = false;
  }
}

Animations::Animations(Adafruit_NeoPixel *strip[], size_t numLeds, size_t numStrips, size_t ballCount, byte fade, bool bMirrored)
    : _strip(strip), _cLength(numLeds), _sLength(numStrips), _cBalls(ballCount), _fadeRate(fade), _bMirrored(bMirrored) {
    
    // Initialize ball colors using strip.Color()
    for (size_t i = 0; i < 5; i++) {
        if (i < _cBalls) {  // Ensure we do not exceed the number of balls
            switch (i) {
                case 0:
                    Colors[i] = _strip[0]->Color(0, 255, 0);   // 1st ball - Blue
                    break;
                case 1:
                    Colors[i] = _strip[0]->Color(0, 0, 255);   // 2nd ball - Green
                    break;
                case 2:
                    Colors[i] = _strip[0]->Color(255, 0, 0);   // 3rd ball - Red
                    break;
                case 3:
                    Colors[i] = _strip[0]->Color(255, 0, 255);   // 4th ball - Yellow
                    break;
                case 4:
                    Colors[i] = _strip[0]->Color(255, 0, 30); // 5th ball - Orange
                    break;
            }
        }
    }

    for (size_t i = 0; i < ballCount; i++) {
        Height[i] = StartHeight;  // Current Ball Height
        ClockTimeAtLastBounce[i] = Time();  // When ball last hit the ground
        Dampening[i] = 0.90 - i / pow(_cBalls, 2);  // Bounciness of this ball
        BallSpeed[i] = InitialBallSpeed(Height[i]);  // Initial launch speed
    }
}


void Animations::BouncingBallEffect() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        // Clear each strip if fading is not applied
        if (_fadeRate != 0) {
            for (size_t i = 0; i < _sLength; i++) {
                _strip[i]->clear();
            }
            Fade();  // Apply fading effect
        } else {
            for (size_t i = 0; i < _sLength; i++) {
                _strip[i]->clear();
            }
        }

        // Draw each of the balls on each strip
        for (size_t j = 0; j < _sLength; j++) { // Iterate through each strip
            for (size_t i = 0; i < _cBalls; i++) {
                double TimeSinceLastBounce = (Time() - ClockTimeAtLastBounce[i]) / SpeedKnob;

                // Calculate the height using the standard acceleration formula
                Height[i] = 0.5 * Gravity * pow(TimeSinceLastBounce, 2.0) + BallSpeed[i] * TimeSinceLastBounce;

                // Ball hits ground - bounce!
                if (Height[i] < 0) {
                    Height[i] = 0;
                    BallSpeed[i] = Dampening[i] * BallSpeed[i];
                    ClockTimeAtLastBounce[i] = Time();

                    if (BallSpeed[i] < 0.01) {
                        BallSpeed[i] = InitialBallSpeed(StartHeight) * Dampening[i];
                    }
                }

                size_t position = (size_t)(Height[i] * (_cLength - 1) / StartHeight);

                // Update the strip with bouncing ball effect
                _strip[j]->setPixelColor(position, Colors[i]);
                if (position + 1 < _cLength) {
                    _strip[j]->setPixelColor(position + 1, Colors[i]);
                }

                if (_bMirrored) {
                    _strip[j]->setPixelColor(_cLength - 1 - position, Colors[i]);
                    if (_cLength - position < _cLength) {
                        _strip[j]->setPixelColor(_cLength - position, Colors[i]);
                    }
                }
            }
        }
        for (size_t i = 0; i < _sLength; i++) {
            _strip[i]->show();
        }
    }
}


uint32_t Animations::getRandomColor() {
  // Generate random values for red, green, and blue
  byte r = random(0, 256);
  byte g = random(0, 256);
  byte b = random(0, 256);
  return _strip[0]->Color(r, b, g);
} 