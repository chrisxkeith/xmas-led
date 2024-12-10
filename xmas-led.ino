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

class Timer {
  private:
    String        msg;
    unsigned long start;
  public:
    Timer(String msg) {
      this->msg = msg;
      start = millis();
    }
    ~Timer() {
      unsigned long ms = millis() - start;
      String msg(ms);
      msg.concat(" milliseconds for ");
      msg.concat(this->msg);
      Serial.println(msg);
    }
};

#include <SparkFun_Qwiic_OLED.h> //http://librarymanager/All#SparkFun_Qwiic_OLED
#include <res/qw_fnt_5x7.h>
#include <res/qw_fnt_8x16.h>
#include <res/qw_fnt_31x48.h>
#include <res/qw_fnt_7segment.h>
#include <res/qw_fnt_largenum.h>

Qwiic1in3OLED myOLED; // 128x64

class OLEDWrapper {
  public:   
    OLEDWrapper(bool stopOnFail) {
      Wire.begin();
      if (!myOLED.begin()) {
        if (stopOnFail) {
          Serial.println("u8g2.begin() failed! Stopping. Try power down/up instead of just restart.");
          while (true) { ; }
        } else {
          Serial.println("u8g2.begin() failed! Continuing.");
        }
      }
      clear();
    }
    void clear() {
      myOLED.erase();
      myOLED.display();
    }
    void addText(uint8_t x, uint8_t y, String s) {
      myOLED.setFont(QW_FONT_8X16);
      myOLED.text(x, y, s.c_str(), COLOR_WHITE);
    }
    void endDisplay() {
      myOLED.display();
    }
    void bitmap(uint8_t x0, uint8_t y0, uint8_t *pBitmap, 
                uint8_t bmp_width, uint8_t bmp_height) {
      myOLED.erase();
      myOLED.bitmap(x0, y0, pBitmap, bmp_width, bmp_height);
      myOLED.display();
    }    
};
OLEDWrapper* oledWrapper = nullptr;

#include <FastLED.h>
const int      NUM_LEDS = 300 - 16;
CRGB           leds[NUM_LEDS] = {0};     // Software gamma mode.

class LEDStripWrapper {
  public:
    static const uint8_t  DATA_PIN = 8;
    static const uint8_t  CLOCK_PIN = 10;
    static void speedTest() {
      Timer timer("speedTest()");
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::White;
        FastLED.show();
        leds[i] = CRGB::Black;
        FastLED.show();
      }
    }
    static void rampTest() {
      Timer timer("rampTest()");
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
    static void blankAll() {
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
      }
      FastLED.show();
    }
    void bitmap(uint8_t x0, uint8_t y0, uint8_t *pBitmap, 
                uint8_t bmp_width, uint8_t bmp_height) {
      uint8_t total = bmp_width * bmp_height;
      for (uint8_t i = 0; i < total; i++) {
        if (pBitmap[x0 + y0 * bmp_width]) {
          leds[i] = CRGB::White;
        } else {
          leds[i] = CRGB::Black;
        }
      }
      FastLED.show();
    }    
};

class App {
  public:
    uint8_t width = 16;
    uint8_t height = 16;

  private:
    bool                  doSpeedTest = false;
    bool                  doRampTest = false;
    bool                  doBitmapTest = false;

    void bitmapTest() {
      const uint8_t SIZE = height * width / 8;
      uint8_t *bits1 = new uint8_t[SIZE];
      {
        Timer t1("bitmapTest");
        for (uint8_t i = 0; i < SIZE; i++) {
          if (i % 2 == 0) {
            bits1[i] = 0xff;
          } else {
            bits1[i] = 0x00;
          }
        }
        oledWrapper->clear();
        oledWrapper->bitmap(0, 0, bits1, width, height);
      }
      delay(2000);
      for (uint8_t i = 0; i < SIZE; i++) {
        bits1[i] = ~bits1[i];
      }
      oledWrapper->bitmap(0, 0, bits1, width, height);
      delay(2000);
      delete bits1;
    }
    void setWidthHeight(String s) {
      int w;
      int h;
      int ret = sscanf(s.c_str(), "w,h=%d,%d", &w, &h);
      width = w;
      height = h;
      if (ret != 2) {
        String err("Error parsing: ");
        err.concat(s);
        err.concat(" (Expected w,h=[width],[height]) Parsed ");
        err.concat(ret);
        err.concat(" items");
        Serial.println(err);
      } else {
        if (width % 8 != 0) {
          width = ((width / 8) + 1) * 8;
        }
        if (height % 8 != 0) {
          height = ((height / 8) + 1) * 8;
        }
        String msg("width = ");
        msg.concat(width);
        msg.concat(" , height = ");
        msg.concat(height);
        Serial.println(msg);
      }
    }
    void checkSerial() {
      if (Serial.available() > 0) {
        String teststr = Serial.readString();  // read until timeout
        teststr.trim();                        // remove any \r \n whitespace at the end of the String
        if (teststr.equals("startSpeedTest")) {
          doSpeedTest = true;
        } else if (teststr.equals("stopSpeedTest")) {
          doSpeedTest = false;
          LEDStripWrapper::blankAll();
        } else if (teststr.equals("startRampTest")) {
          doRampTest = true;
        } else if (teststr.equals("stopRampTest")) {
          doRampTest = false;
          LEDStripWrapper::blankAll();
        } else if (teststr.equals("startBitmapTest")) {
          doBitmapTest = true;
        } else if (teststr.equals("stopBitmapTest")) {
          doBitmapTest = false;
          oledWrapper->clear();
        } else if (teststr.equals("blankAll")) {
          LEDStripWrapper::blankAll();
        } else if (teststr.startsWith("w,h=")) {
          setWidthHeight(teststr);
        } else {
          String msg("Unknown command: '");
          msg.concat(teststr);
          msg.concat("'. Expected one of startSpeedTest, stopSpeedTest, "
                    "startRampTest, stopRampTest, startBitmapTest, stopBitmapTest, "
                    "blankAll, w,h=[width],[height]");
          Serial.println(msg);
        }
      }
    }
  public:
    void setup() {
      Timer timer("setup()");
      delay(500); // power-up safety delay
      Serial.begin(115200);
      Serial.println("setup() started.");
      Wire.begin();
      FastLED.addLeds<APA102, LEDStripWrapper::DATA_PIN, LEDStripWrapper::CLOCK_PIN, BGR>(leds, NUM_LEDS);
      {
        Timer t("LEDStripWrapper::blankAll()");
        LEDStripWrapper::blankAll();
      }
      // Utils::scanI2C();
      oledWrapper = new OLEDWrapper(false);
      oledWrapper->clear();
      oledWrapper->addText(0, 0, String("setup() finished."));
      oledWrapper->endDisplay();
    }
    void loop() {
      if (doSpeedTest)  { LEDStripWrapper::speedTest(); }
      if (doRampTest)   { LEDStripWrapper::rampTest(); }
      if (doBitmapTest) { bitmapTest(); }
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
