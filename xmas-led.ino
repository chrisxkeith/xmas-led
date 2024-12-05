#include <FastLED.h>

class App {
  private:
    static const uint8_t  DATA_PIN = 8;
    static const uint8_t  CLOCK_PIN = 10;
    static const int      NUM_LEDS = 300;
    CRGB                  leds[NUM_LEDS] = {0};     // Software gamma mode.;
    bool                  doSpeedTest = false;
    bool                  doIntensityTest = false;
    bool                  doRampTest = false;
    bool                  numLEDSToTry = 2;

    void speedTest() {
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::White;
        FastLED.show();
        leds[i] = CRGB::Black;
        FastLED.show();
      }
    }
    void rampTest() {
      // Draw a linear ramp of brightnesses to showcase the difference between
      // the HD and non-HD mode.
      for (int i = 0; i < 10; i++) {
          uint8_t brightness = map(i, 0, NUM_LEDS - 1, 0, 255);
          CRGB c(brightness, brightness, brightness);  // Just make a shade of white.
          leds[i] = c;
      }
      FastLED.show();  // All LEDs are now displayed.
      delay(8);  // Wait 8 milliseconds until the next frame.
    }
    void intensityTest() {
      for (int i = 0; i < numLEDSToTry; i++) {
        leds[i] = CRGB::White;
        delay(10);
      }
      FastLED.show();
      delay(2000);
      for (int i = 0; i < numLEDSToTry; i++) {
        leds[i] = CRGB::Black;
        delay(10);
      }
      FastLED.show();
      delay(2000);
    }
    void blankAll() {
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
        FastLED.show();
      }
    }
    void checkSerial() {
      if (Serial.available() > 0) {
        String teststr = Serial.readString();  //read until timeout
        teststr.trim();                        // remove any \r \n whitespace at the end of the String
        if (teststr.equals("startSpeedTest")) {
          doSpeedTest = true;
        } else if (teststr.equals("stopSpeedTest")) {
          doSpeedTest = false;
          blankAll();
        } else if (teststr.equals("startIntensityTest")) {
          doIntensityTest = true;
        } else if (teststr.equals("stopIntensityTest")) {
          doIntensityTest = false;
          blankAll();
        } else if (teststr.equals("startRampTest")) {
          doRampTest = true;
        } else if (teststr.equals("stopRampTest")) {
          doRampTest = false;
          blankAll();
        } else if (teststr.equals("blankAll")) {
          blankAll();
        } else {
          String msg("Unknown command: '");
          msg.concat(teststr);
          msg.concat("'. Expected one of startSpeedTest, stopSpeedTest, "
                    "startIntensityTest, stopIntensityTest, startRampTest, stopRampTest, "
                    "blankAll");
          Serial.println(msg);
        }
      }
    }

  public:
    void setup() {
      delay(500); // power-up safety delay
      Serial.begin(115200);
      FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
      blankAll();
    }
    void loop() {
      if (doSpeedTest) {
        speedTest();
      }
      if (doIntensityTest) {
        intensityTest();
      }
      if (doRampTest) {
        rampTest();
      }
      checkSerial();
    }
};
App app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}
