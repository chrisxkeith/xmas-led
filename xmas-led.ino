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

#include <cstring>
// 1 bit deep, e.g., monochrome
class Bitmap {
  public:
    uint8_t   width;
    uint8_t   height;
    uint8_t*  bitmap;
    Bitmap(uint8_t width, uint8_t height) {
      if (width % 8 != 0) {
        width = ((width / 8) + 1) * 8;
      }
      if (height % 8 != 0) {
        height = ((height / 8) + 1) * 8;
      }
      this->width = width;
      this->height = height;
      bitmap = new uint8_t[sizeInBytes()];
      clear();
    }
    void clear() {
      std::memset(bitmap, 0b0000, sizeInBytes());
    }
    int sizeInBytes() { return width * height / 8; }
    int calcByteIndex(uint8_t x, uint8_t y) {
      int i = (x / 8) + (y * width / 8);
      if (i >= sizeInBytes() * 8) {
        String err("x=");
        err.concat(x);
        err.concat(", y=");
        err.concat(y);
        err.concat(" out of bounds ");
        err.concat(width);
        err.concat(",");
        err.concat("height");
        Serial.println(err);
        return 1;
      }
      return i;
    }
    void setBit(uint8_t x,  uint8_t y) {
      int i = calcByteIndex(x, y);
      bitmap[i] |= (1 << x);
    }
    void clearBit(uint8_t x,  uint8_t y) {
      int i = calcByteIndex(x, y);
      bitmap[i] &= ~(1 << x);
    }
    int getBit(uint8_t x, uint8_t y) {
      byte b = bitmap[calcByteIndex(x, y)];
      bool bit = (b >> x) & 0x1;
      return bit;
    }
    void dump() {
      String msg;
      for (int y = 0; y < height; y++) {
        msg.remove(0);
        for (int x = 0; x < width / 8; x++) {
          byte b = bitmap[calcByteIndex(x, y)];
          for (int i = 7; i >= 0; i--) {
            msg.concat((b >> i) & 1);
          }
          msg.concat(" ");
        }
        Serial.println(msg);
      }
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
    void showBitmap(Bitmap *pBitmap) {
      int ledIndex = 0;
      for (uint8_t x = 0; x < pBitmap->width; x++) {
        for (uint8_t y = 0; y < pBitmap->height; y++) {
          if (pBitmap->getBit(x, y)) {
            leds[ledIndex++] = CRGB::White;
          } else {
            leds[ledIndex++] = CRGB::Black;
          }
        }
        if (ledIndex >= NUM_LEDS) {
          String err("Went beyond length of LED=");
          err.concat(NUM_LEDS);
          err.concat(" , ledIndex=");
          err.concat(ledIndex);
          Serial.println(err);
          break;
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

    String configs[4] = {
      "~2024Dec12:10:52", // date +"%Y%b%d:%H:%M"
      "https://github.com/chrisxkeith/xmas-led",
    };

    void bitmapTest() {
      Bitmap  bm(width, height);
      {
        Timer t1("bitmapTest");
        for (uint8_t x = 0; x < width; x++) {
          for (uint8_t y = 0; y < height; y++) {
            if (x % 2) {
              if (y % 2) {
                bm.setBit(x, y);
              }
            } else {
              if (y % 2 == 0) {
                bm.setBit(x, y);
              }
            }
          }
        }
        oledWrapper->clear();
        oledWrapper->bitmap(0, 0, bm.bitmap, width, height);
      }
      delay(2000);
      for (uint8_t x = 0; x < width; x++) {
        for (uint8_t y = 0; y < height; y++) {
          if (bm.getBit(x, y)) {
            bm.clearBit(x, y);
          } else {
            bm.setBit(x, y);
          }
        }
      }
      oledWrapper->bitmap(0, 0, bm.bitmap, width, height);
      delay(2000);
      bm.clear();
      for (uint8_t x = 0; x < width; x++) {
        for (uint8_t y = 0; y < height; y++) {
          if (x == y) {
            bm.setBit(x, y);
          }
        }
      }
      oledWrapper->bitmap(0, 0, bm.bitmap, width, height);
      delay(2000);
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
      oledWrapper->addText(0, 0, configs[0]);
      oledWrapper->endDisplay();
      delay(4000);
      oledWrapper->clear();
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
