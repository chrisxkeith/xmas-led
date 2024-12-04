#include <FastLED.h>

#define DATA_PIN 8
#define CLOCK_PIN 10

class App {
  private:
    const int         DELAY = 0;
    static const int  NUM_LEDS = 300;
    CRGB              leds[NUM_LEDS];

    void speedTest() {
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::White;
        FastLED.show();
        delay(DELAY);
        leds[i] = CRGB::Black;
        FastLED.show();
        delay(DELAY);
      }
    }
  public:
    void setup() {
      Serial.begin(115200);
      delay(1000);
      Serial.println("setup()");
      FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
    }
    void loop() {
      speedTest();
    }
};
App app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}
