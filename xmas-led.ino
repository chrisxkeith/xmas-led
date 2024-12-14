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

#include <cstring> // for memset()
// Horizontal bytes, left-to-right, top-to-bottom.
// Most-significant bit of first byte == (0,0).
// 1 bit deep, e.g., monochrome
class Bitmap {
  public:
    int       width;
    int       height;
    uint8_t*  bitmap;
    Bitmap(int width, int height) {
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
    int calcByteIndex(int x, int y) {
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
    void setBit(int x,  int y) {
      int i = calcByteIndex(x, y);
      bitmap[i] |= (1 << (7 - (x % 8)));
    }
    void clearBit(int x,  int y) {
      int i = calcByteIndex(x, y);
      bitmap[i] &= ~(1 << (7 - (x % 8)));
    }
    int getBit(int x, int y) {
      byte b = bitmap[calcByteIndex(x, y)];
      bool bit = (b >> x) & 0x1;
      return bit;
    }
    void dump(String title) {
      Serial.println(title);
      String msg;
      for (int y = 0; y < height; y++) {
        msg.remove(0);
        for (int x = 0; x < width; x += 8) {
          byte b = bitmap[calcByteIndex(x, y)];
          for (int i = 7; i >= 0; i--) {
            int bit = (b >> i) & 1;
            char c = (bit ? '1' : '.');
            msg.concat(c);
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
    void addText(int x, int y, String s) {
      myOLED.setFont(QW_FONT_8X16);
      myOLED.text(x, y, s.c_str(), COLOR_WHITE);
    }
    void endDisplay() {
      myOLED.display();
    }
    // Must convert to OLED bitmap format: vertical bytes, left-to-right, top-to-bottom
    // Least-significant bit of first byte == (0,0).
    void bitmap(int x0, int y0, uint8_t *pBitmap, 
                int bmp_width, int bmp_height) {
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
    static const int  DATA_PIN = 8;
    static const int  CLOCK_PIN = 10;
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
          int brightness = map(i, 0, NUM_TO_TEST - 1, 0, 255);
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
      for (int x = 0; x < pBitmap->width; x++) {
        for (int y = 0; y < pBitmap->height; y++) {
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

#define BMP_TRUCK_WIDTH  19
#define BMP_TRUCK_HEIGHT 16

static uint8_t bmp_truck_data[] = {
    0xFF,
    0x01,
    0xC1,
    0x41,
    0x41,
    0x41,
    0x71,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x71,
    0x41,
    0x41,
    0xC1,
    0x81,
    0x01,
    0xFF,
    
    0xFF,
    0x80,
    0x83,
    0x82,
    0x86,
    0x8F,
    0x8F,
    0x86,
    0x82,
    0x82,
    0x82,
    0x86,
    0x8F,
    0x8F,
    0x86,
    0x83,
    0x81,
    0x80,
    0xFF,
}; // 38 bytes

class App {
  public:
    int width = 16;
    int height = 16;

  private:
    bool                  doSpeedTest = false;
    bool                  doRampTest = false;
    bool                  doOledBitmapTest = false;

    String configs[4] = {
      "~2024Dec14:08:36", // date +"%Y%b%d:%H:%M"
      "https://github.com/chrisxkeith/xmas-led",
    };

    void truckTest() {
      oledWrapper->bitmap(0, 0, bmp_truck_data, BMP_TRUCK_WIDTH, BMP_TRUCK_HEIGHT);
    }
    void runTest(Bitmap* bm, String title) {
      if (doOledBitmapTest) {
        oledWrapper->bitmap(0, 0, bm->bitmap, width, height);
        delay(2000);
      } else {
        bm->dump(title);
      }
      bm->clear();
    }
    void bitmapTest() {
      Bitmap  bm(width, height);
      for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
          if (x == y) {
            bm.setBit(x, y);
          }
        }
      }
      runTest(&bm, "diagonal");
      bm.clear();
      for (int x = 0; x < width; x++) {
          bm.setBit(x, 0);
      }
      runTest(&bm, "horizontal");
      bm.clear();
      for (int y = 0; y < height; y++) {
        bm.setBit(0, y);
      }
      runTest(&bm, "vertical");
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
        } else if (teststr.equals("startOledBitmapTest")) {
          doOledBitmapTest = true;
        } else if (teststr.equals("stopOledBitmapTest")) {
          doOledBitmapTest = false;
        } else if (teststr.equals("runBitmapTest")) {
          bitmapTest();
        } else if (teststr.equals("blankAll")) {
          LEDStripWrapper::blankAll();
        } else if (teststr.startsWith("w,h=")) {
          setWidthHeight(teststr);
        } else {
          String msg("Unknown command: '");
          msg.concat(teststr);
          msg.concat("'. Expected one of startSpeedTest, stopSpeedTest, "
                    "startRampTest, stopRampTest, runBitmapTest, "
                    "startOledBitmapTest, stopOledBitmapTest, "
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
      if (doSpeedTest)      { LEDStripWrapper::speedTest(); }
      if (doRampTest)       { LEDStripWrapper::rampTest(); }
      if (doOledBitmapTest) { bitmapTest(); }
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
