#include <FastLED.h>

class App {
  private:
    static const uint8_t  DATA_PIN = 8;
    static const uint8_t  CLOCK_PIN = 10;
    static const int      NUM_LEDS = 300;
    CRGB                  leds[NUM_LEDS];
    bool                  doSpeedTest = false;
    bool                  doIntensityTest = false;
    bool                  numLEDSToTry = 2;

    void speedTest() {
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::White;
        FastLED.show();
        leds[i] = CRGB::Black;
        FastLED.show();
      }
    }
    void intensityTest() {
      const int NUM_LEDS_TO_TRY = 10;
      for (int i = 0; i < numLEDSToTry; i++) {
        leds[i] = CRGB::White;
        FastLED.show();
      }
      delay(2000);
      for (int i = 0; i < numLEDSToTry; i++) {
        leds[i] = CRGB::Black;
        FastLED.show();
      }
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
        } else if (teststr.equals("startIntensityTest")) {
          doIntensityTest = true;
        } else if (teststr.equals("stopIntensityTest")) {
          doIntensityTest = false;
        } else {
          String msg("Unknown command: '");
          msg.concat(teststr);
          msg.concat("'. Expected one of startSpeedTest, stopSpeedTest, startIntensityTest, stopIntensityTest");
          Serial.println(msg);
        }
      }
    }

  public:
    void setup() {
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
