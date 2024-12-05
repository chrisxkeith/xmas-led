#include <Wire.h>
#include <vector>
#include <set>

class Utils {
  public:
    // Modified from https://playground.arduino.cc/Main/I2cScanner/
    static void scanI2C() {
      Serial.println("I2C: Scanning for devices...");    
      std::vector<byte> foundDevices;
      std::set<byte> errors;
      for( byte address = 1; address < 127; address++ ) {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
    
        if (error == 0) {
          foundDevices.push_back(address);
        } else {
          errors.insert(error);
        }    
      }
      Serial.print("I2C: Devices found at: ");
      for (byte b : foundDevices) {
        Serial.print("0x");
        if (b < 16) {
          Serial.print("0");
        }
        Serial.print(b, HEX);
        Serial.print(" ");
      }
      Serial.println("");
      String ss("I2C: Error numbers returned: ");
      std::set<byte>::iterator itr;
      for (itr = errors.begin(); itr != errors.end(); itr++) {
        ss.concat(*itr);
        ss.concat(" ");
      }
      Serial.println(ss);
    }
};

#include <SparkFun_Qwiic_OLED.h> //http://librarymanager/All#SparkFun_Qwiic_OLED
#include <res/qw_fnt_8x16.h>
#include <res/qw_fnt_largenum.h>
Qwiic1in3OLED u8g2; // 128x64

class OLEDWrapper {
  private:
      const int START_BASELINE = 20;
      int       baseLine = START_BASELINE;
  public:   
    void setup_OLED() {
      Wire.begin();
      delay(2000); // try to avoid u8g2.begin() failure.
      if (!u8g2.begin()) {
        Serial.println("u8g2.begin() failed! Stopping. Try power down/up instead of just restart.");
        while (true) { ; }
      }
      clear();
    }
    void clear() {
      u8g2.erase();
      u8g2.display();
    }
    void display(String s, uint8_t x, uint8_t y) {
      u8g2.setFont(QW_FONT_8X16);
      u8g2.text(x, y, s);
    }
    void endDisplay() {
      u8g2.display();
    }
    void shiftDisplay(int shiftAmount) {
        baseLine += shiftAmount;
        if (baseLine > 63) {
          baseLine = START_BASELINE;
        }
    }
};
OLEDWrapper* oledWrapper = nullptr;

#include <FastLED.h>

class App {
  private:
    static const uint8_t  DATA_PIN = 8;
    static const uint8_t  CLOCK_PIN = 10;
    static const int      NUM_LEDS = 300;
    CRGB                  leds[NUM_LEDS] = {0};     // Software gamma mode.;
    bool                  doSpeedTest = false;
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
      const int NUM_TO_TEST = NUM_LEDS;
      for (int i = 0; i < NUM_TO_TEST; i++) {
          uint8_t brightness = map(i, 0, NUM_TO_TEST - 1, 0, 255);
          CRGB c(brightness, brightness, brightness);  // Just make a shade of white.
          leds[i] = c;
      }
      FastLED.show();  // All LEDs are now displayed.
      delay(8);  // Wait 8 milliseconds until the next frame.
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
                    "startRampTest, stopRampTest, "
                    "blankAll");
          Serial.println(msg);
        }
      }
    }

  public:
    void setup() {
      delay(500); // power-up safety delay
      Serial.begin(115200);
      Serial.println("setup() started.");
      Wire.begin();
      FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
      blankAll();
      Utils::scanI2C();
      oledWrapper = new OLEDWrapper();
      // oledWrapper->display(String("setup() finished."), 0, FONT_8X16_HEIGHT);
      Serial.println("setup() finished.");
    }
    void loop() {
      if (doSpeedTest) { speedTest(); }
      if (doRampTest) { rampTest(); }
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
