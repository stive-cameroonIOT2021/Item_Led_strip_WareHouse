#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))  // Count elements in a static array

// Class for Bouncing Ball Effect
class Animations {
  private:
    size_t _cLength;
    size_t _sLength;
    size_t _cBalls;
    byte _fadeRate;
    bool _bMirrored;
    Adafruit_NeoPixel **_strip;  // Pointer to an array of pointers to Adafruit_NeoPixel objects

    const double Gravity = -9.81;  // Gravity constant
    const double StartHeight = 1;  // Initial height
    const double SpeedKnob = 4.0;  // Speed control factor

    double ClockTimeAtLastBounce[5], Height[5], BallSpeed[5], Dampening[5];
    uint32_t Colors[5];

    unsigned long previousMillis = 0, previousMillis1 = 0;
    const unsigned long interval = 20;  // Update interval in milliseconds

    int deltaHue;
    byte hue = 0;
    int iDirection = 1;
    int iPos = 0;

    unsigned long previousMillisMarquee;
    unsigned long previousMillisMarqueeMirrored;

    unsigned long previousMillisTwinkle1;
    
    static const uint32_t TwinkleColors[5];  // Twinkle colors

    static double Time() {
        return millis() / 1000.0;  // Convert milliseconds to seconds
    }

    double InitialBallSpeed(double height) const {
        return sqrt(-2 * Gravity * height);  // Calculate initial ball speed based on height
    }

    void Fade() {
        for (size_t stripIndex = 0; stripIndex < _sLength; stripIndex++) {
            Adafruit_NeoPixel &strip = *_strip[stripIndex];
            for (size_t i = 0; i < _cLength; i++) {
                uint32_t color = strip.getPixelColor(i);
                byte r = (color >> 16) & 0xFF;
                byte g = (color >> 8) & 0xFF;
                byte b = color & 0xFF;
                r = max(r - _fadeRate, 0);
                g = max(g - _fadeRate, 0);
                b = max(b - _fadeRate, 0);
                strip.setPixelColor(i, strip.Color(r, b, g));
            }
        }
    }

    uint32_t getRandomColor();
    void DrawMarquee();
    void DrawMarqueeMirrored();
    void clearStrip(size_t stripIndex);

  public:
    Animations(Adafruit_NeoPixel *strip[], size_t numLeds, size_t numStrips, size_t ballCount, byte fade = 0, bool bMirrored = false);
    void BouncingBallEffect();
    void colorWipe(unsigned long wipeTime);
    void CometEffect(int cometSize, int deltaHue, unsigned long interval);
    void DrawMarqueeDrawMarqueeMirrored(unsigned long interval);
    void drawTwinkle(unsigned long TWINKLE_DELAY);
    void colorWipe2(unsigned long wipeTime);
};