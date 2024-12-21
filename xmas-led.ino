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
      bool bit = b >> (7 - (x % 8)) & 0x1;
      return bit;
    }
    void dump(String title) {
      Serial.println(title);
      String msg;
      for (int y = 0; y < height; y++) {
        msg.remove(0);
        for (int x = 0; x < width; x++) {
          char c = (getBit(x, y) ? '1' : '.');
          msg.concat(c);
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
    // Brute-force loops over bytes, or get/setBit?
    void bitmap(int x0, int y0, uint8_t *pBitmap, 
                int bmp_width, int bmp_height) {
      myOLED.erase();
      myOLED.bitmap(x0, y0, pBitmap, bmp_width, bmp_height);
      myOLED.display();
    }
};
OLEDWrapper* oledWrapper = nullptr;

#include <FastLED.h>
const int     LEDS_WIDTH = 16;
const int     LEDS_HEIGHT = 16;
const int     NUM_LEDS = LEDS_WIDTH * LEDS_HEIGHT;
CRGB          leds[NUM_LEDS] = {0};     // Software gamma mode.

class LEDStripWrapper {
  private:
    static int pixelToLedIndex[NUM_LEDS];
  public:
    static const int  DATA_PIN = 8;
    static const int  CLOCK_PIN = 10;
    static uint32_t   theDelay;
    static void speedTest() {
      Timer timer("speedTest()");
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::White;
        FastLED.show();
        delay(theDelay);
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
    static void clear() {
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
      }
      FastLED.show();
    }
    static void showBitmap(Bitmap *pBitmap) {
      if (pBitmap->width != LEDS_WIDTH || pBitmap->height != LEDS_WIDTH) {
        String err("bitmap must be 16 x 16");
        Serial.println(err);
        clear();
        speedTest();
        clear();
        return;
      }
      for (int x = 0; x < pBitmap->width; x++) {
        for (int y = 0; y < pBitmap->height; y++) {
          if (pBitmap->getBit(x, y)) {
            leds[pixelToLedIndex[(y * LEDS_WIDTH) + x]] = CRGB::White;
          } else {
            leds[pixelToLedIndex[(y * LEDS_WIDTH) + x]] = CRGB::Black;
          }
        }
      }
      FastLED.show();
    }
    static void setDelay(String s) {
      int d;
      int ret = sscanf(s.c_str(), "theDelay=%d", &d);
      if (ret != 1) {
        String err("Error parsing: ");
        err.concat(s);
        err.concat(" (Expected theDelay=[delay]) Parsed ");
        err.concat(ret);
        err.concat(" items");
        Serial.println(err);
      } else {
        theDelay = d;
      }
    }
    static void startup() {
      clear();
      leds[0] = CRGB::White;
      leds[NUM_LEDS - 1] = CRGB::White;
      FastLED.show();
      leds[0] = CRGB::Black;
      leds[NUM_LEDS - 1] = CRGB::Black;
      FastLED.show();
    }
};
uint32_t LEDStripWrapper::theDelay = 0;
int LEDStripWrapper::pixelToLedIndex[NUM_LEDS] = {
   15,  14,  13,  12,  11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0,
   16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
   47,  46,  45,  44,  43,  42,  41,  40,  39,  38,  37,  36,  35,  34,  33,  32,
   48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
   79,  78,  77,  76,  75,  74,  73,  72,  71,  70,  69,  68,  67,  66,  65,  64,
   80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
  111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100,  99,  98,  97,  96,
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127
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

class XmasDisplayer {
  private:
    const int     FRAME_RATE = 1000 / 24;
    unsigned long lastDisplay = 0;
    Bitmap*       bitmap;

    void convertToOLED() {
      // to come
    }
  public:
    int width = 16;
    int height = 16;

    XmasDisplayer() {
      bitmap = new Bitmap(width, height);
    }
    void display() {
      unsigned long now = millis();
      if (now < lastDisplay) {
        lastDisplay = 0;
      }
      if (now - lastDisplay < FRAME_RATE) {
        return;
      }
    }
    void runTest(String title, bool showOLED, bool showLEDStrip, bool showTextBitmap) {
      if (showOLED) {
        oledWrapper->bitmap(0, 0, bitmap->bitmap, width, height);
        delay(2000);
      }
      if (showLEDStrip) {
        LEDStripWrapper::showBitmap(bitmap);
        delay(2000);
        LEDStripWrapper::clear();
      }
      if (showTextBitmap) {
        bitmap->dump(title);
      }
      bitmap->clear();
    }
    void bitmapTest(bool showOLED, bool showLEDStrip, bool showTextBitmap) {
      for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
          if (x == y) {
            bitmap->setBit(x, y);
            bitmap->setBit(15 - x, y);
          }
        }
      }
      for (int x = 1; x < width - 1; x++) {
        bitmap->setBit(x, 0);
        bitmap->setBit(x, 15);
      }
      for (int y = 1; y < height - 1; y++) {
        bitmap->setBit(0, y);
        bitmap->setBit(15, y);
      }
      runTest("diagonals", showOLED, showLEDStrip, showTextBitmap);
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
};
XmasDisplayer xmasDisplayer;

class App {
  private:
    bool  doSpeedTest = false;
    bool  doRampTest = false;
    bool  doBitmapTest = false;
    bool  showOLED = false;
    bool  showLEDStrip = false;
    bool  showTextBitmap = false;

    String cmds = "startSpeedTest, stopSpeedTest, "
                    "startRampTest, stopRampTest, runBitmapTest, "
                    "startBitmapTest, stopBitmapTest, "
                    "showOLED, showLEDStrip, showTextBitmap, "
                    "hideOLED, hideLEDStrip, hideTextBitmap, "
                    "clear, w,h=[width],[height], theDelay=[delayInMilliseconds], "
                    "showBuild";
    String configs[4] = {
      "~2024Dec20:14:13", // date +"%Y%b%d:%H:%M"
      "https://github.com/chrisxkeith/xmas-led",
    };

    void truckTest() {
      oledWrapper->bitmap(0, 0, bmp_truck_data, BMP_TRUCK_WIDTH, BMP_TRUCK_HEIGHT);
    }
    void showBuild() {
      oledWrapper->clear();
      oledWrapper->addText(0, 0, configs[0]);
      oledWrapper->endDisplay();
    }
    void checkSerial() {
      if (Serial.available() > 0) {
        String teststr = Serial.readString();  // read until timeout
        teststr.trim();                        // remove any \r \n whitespace at the end of the String
        if (teststr.equals("startSpeedTest")) {
          doSpeedTest = true;
        } else if (teststr.equals("stopSpeedTest")) {
          doSpeedTest = false;
          LEDStripWrapper::clear();
        } else if (teststr.equals("startRampTest")) {
          doRampTest = true;
        } else if (teststr.equals("stopRampTest")) {
          doRampTest = false;
          LEDStripWrapper::clear();
        } else if (teststr.equals("startBitmapTest")) {
          doBitmapTest = true;
        } else if (teststr.equals("stopBitmapTest")) {
          doBitmapTest = false;
          oledWrapper->clear();
          LEDStripWrapper::clear();
        } else if (teststr.equals("runBitmapTest")) {
          xmasDisplayer.bitmapTest(showOLED, showLEDStrip, showTextBitmap);
        } else if (teststr.equals("clear")) {
          LEDStripWrapper::clear();
          oledWrapper->clear();
        } else if (teststr.startsWith("w,h=")) {
          xmasDisplayer.setWidthHeight(teststr);
        } else if (teststr.startsWith("theDelay=")) {
          LEDStripWrapper::setDelay(teststr);
        } else if (teststr.startsWith("showOLED")) {
          showOLED = true;
        } else if (teststr.startsWith("showLEDStrip")) {
          showLEDStrip = true;
        } else if (teststr.startsWith("showTextBitmap")) {
          showTextBitmap = true;
        } else if (teststr.startsWith("hideOLED")) {
          showOLED = false;
          oledWrapper->clear();
        } else if (teststr.startsWith("hideLEDStrip")) {
          showLEDStrip = false;
          LEDStripWrapper::clear();
        } else if (teststr.startsWith("hideTextBitmap")) {
          showTextBitmap = false;
        } else if (teststr.startsWith("showBuild")) {
          showBuild();
        } else {
          String msg("Unknown command: '");
          msg.concat(teststr);
          msg.concat("'. Expected one of ");
          msg.concat(cmds);
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
        Timer t("LEDStripWrapper::startup()");
        LEDStripWrapper::startup();
      }
      // Utils::scanI2C();
      oledWrapper = new OLEDWrapper(false);
      oledWrapper->clear();
      Serial.println(cmds.c_str());
    }
    void loop() {
      if (doSpeedTest)  { LEDStripWrapper::speedTest(); }
      if (doRampTest)   { LEDStripWrapper::rampTest(); }
      if (doBitmapTest) { xmasDisplayer.bitmapTest(showOLED, showLEDStrip, showTextBitmap); }
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
