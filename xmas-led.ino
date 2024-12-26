#include <Wire.h>
#include <vector>
#include <set>

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

class Utils {
  public:
    // Modified from https://playground.arduino.cc/Main/I2cScanner/
    static void scanI2C() {
      Timer t("scanI2C()");
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
      Serial.print(", ");
      String ss("Error numbers returned: ");
      std::set<byte>::iterator itr;
      for (itr = errors.begin(); itr != errors.end(); itr++) {
        ss.concat(*itr);
        ss.concat(" ");
      }
      Serial.println(ss);
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
        err.concat(" out of bounds (");
        err.concat(width);
        err.concat(",");
        err.concat(height);
        err.concat(")");
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
    void createDiagonals() {
      clear();
      for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
          if (x == y) {
            setBit(x, y);
            setBit(width - 1 - x, y);
          }
        }
      }
      for (int x = 1; x < width - 1; x++) {
        setBit(x, 0);
        setBit(x, height - 1);
      }
      for (int y = 1; y < height - 1; y++) {
        setBit(0, y);
        setBit(width - 1, y);
      }
    }
    void fill() {
      clear();
      for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
          setBit(x, y);
        }
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
  private:
    const int SIZE_IN_BYTES = kOLED1in3Width * kOLED1in3Height / 8;
    uint8_t*  oledBitmap = new uint8_t[SIZE_IN_BYTES];

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
    // Must convert to OLED bitmap format: vertical bytes, left-to-right, top-to-bottom
    // Least-significant bit of first byte == (0,0).
    void bitmap(Bitmap *pBitmap) {
      std::memset(oledBitmap, 0b0000, SIZE_IN_BYTES);
      for (int x = 0; x < pBitmap->width; x++) {
        for (int y = 0; y < pBitmap->height; y++) {
          if (pBitmap->getBit(x, y)) {
            int   byteIndex = x + (y / 8 * kOLED1in3Width);
            oledBitmap[byteIndex] |= (1 << (y % 8));
          } 
        }
      }
      rawBitmap(oledBitmap, kOLED1in3Width, kOLED1in3Height);
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
    void rawBitmap(uint8_t *pBitmap, uint8_t width, uint8_t height) {
      myOLED.erase();
      myOLED.bitmap(0, 0, pBitmap, width, height);
      myOLED.display();
    }
};
OLEDWrapper* oledWrapper = nullptr;

#include <FastLED.h>
const int     LEDS_WIDTH = 16;
const int     LEDS_HEIGHT = 16;
const int     NUM_LEDS = LEDS_WIDTH * LEDS_HEIGHT;
CRGB          leds[NUM_LEDS] = {0};     // Software gamma mode.

#define HALF_WHITE 0x808080
#define QUARTER_WHITE 0x404040
#define EIGHTH_WHITE 0x202020
#define WHITENESS EIGHTH_WHITE
class LEDStripWrapper {
  private:
    static int pixelToLedIndex[NUM_LEDS];
  public:
    static const int  DATA_PIN = 8;   // green
    static const int  CLOCK_PIN = 10; // blue
    static uint32_t   theDelay;
    static void speedTest() {
      Timer timer("speedTest()");
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = WHITENESS;
        FastLED.show();
        delay(theDelay);
        leds[i] = CRGB::Black;
        FastLED.show();
      }
    }
    static void rampTestForward() {
      Timer timer("rampTestForward()");
      // Draw a linear ramp of brightnesses to showcase the difference between
      // the HD and non-HD mode.
      const int NUM_TO_TEST = NUM_LEDS;
      for (int i = 0; i < NUM_TO_TEST; i++) {
          int brightness = map(i, 0, NUM_TO_TEST - 1, 0, 255);
          CRGB c(brightness, brightness, brightness);  // Just make a shade of white.
          leds[i] = c;
      }
      FastLED.show();  // All LEDs are now displayed.
      delay(5000);
      clear();
    }
    static void rampTestBackward() {
      Timer timer("rampTestBackward()");
      // Draw a linear ramp of brightnesses to showcase the difference between
      // the HD and non-HD mode.
      const int NUM_TO_TEST = NUM_LEDS;
      for (int i = NUM_TO_TEST - 1; i > -1; i--) {
          int brightness = map(i, 0, NUM_TO_TEST - 1, 0, 255);
          CRGB c(brightness, brightness, brightness);  // Just make a shade of white.
          leds[i] = c;
      }
      FastLED.show();  // All LEDs are now displayed.
      delay(5000);
      clear();
    }
    static void capacityTest() {
      for (int t = 0; t < 15; t++) {
        Bitmap b(16, 16);
        for (int y = 0; y < t; y++) {
          for (int x = 0; x < 16; x++) {
            b.setBit(x, y);
          }
        }
        for (int x = 0; x < 16; x++) {
          b.setBit(x, 15);
        }
        showBitmap(&b);
        delay(1000);
        clear();
      }
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
            leds[pixelToLedIndex[(y * LEDS_WIDTH) + x]] = WHITENESS;
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
      speedTest();
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
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
  143, 142, 141, 140, 139, 138, 137, 136, 135, 134, 133, 132, 131, 130, 129, 128,
  144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
  175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, 160,
  176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
  207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 194, 193, 192,
  208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
  239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224,
  240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

class Snowflake {
  public:
    int             currentX;
    int             currentY;
    int             velocityInMS;
    unsigned long   lastRedraw;
    Snowflake(int currentX, int currentY, int velocityInMS) {
      this->currentX = currentX;
      this->currentY = currentY;
      this->velocityInMS = velocityInMS;
      this->lastRedraw = 0;      
    }
    void dump() {
      String s("x: ");
      s.concat(currentX);
      s.concat(", y: ");
      s.concat(currentY);
      s.concat(", v: ");
      s.concat(velocityInMS);
      s.concat(", last: ");
      s.concat(lastRedraw);
      Serial.println(s);
    }
};

using namespace std;
#include <bitset>

class RandomDistributor {
  private:
    const static int      N_COORDS = 16;
    std::bitset<N_COORDS> xcoords{0};
  public:
    void reset() {
      xcoords.reset();
    }
    int getNextCoord() {
      int x = rand() % N_COORDS;
      while (xcoords[x]) {
        x = rand() % N_COORDS;
      }
      xcoords[x] = true;
      return x;
    }
};

class XmasDisplayer {
  private:
    const static int   WIDTH = 16;
    const static int   HEIGHT = 16;

    Bitmap*            bitmap;
    vector<Snowflake>  snowflakes;
    bool               show = true;
    int                endFlakeCount = 16;
    long               minVelocity = 300;
    int                maxVelocity = 1500;
    unsigned long      lastAddedTime = 0;
    RandomDistributor  flakeDistributor;
    RandomDistributor  meltDistributor;
    int                snowLevel[WIDTH];
    bool               snowing = true;

    Snowflake createSnowflake() {
      int v = rand() % (maxVelocity - minVelocity) + minVelocity;
      return Snowflake(flakeDistributor.getNextCoord(), -1,  v);
    }
    void restart() {
      meltDistributor.reset();
      for (int i = 0; i < WIDTH; i++) {
        snowLevel[i] = HEIGHT;
      }
      snowing = true;
    }
  public:
    XmasDisplayer() {
      bitmap = new Bitmap(WIDTH, HEIGHT);
      restart();
    }
    void clear() {
      if (show) {
        LEDStripWrapper::clear();
      }
      show = ! show;
    }
    void melt(unsigned long now) {
      for (std::vector<Snowflake>::iterator it = snowflakes.begin(); it != snowflakes.end(); ++it) {
        bitmap->clearBit(it->currentX, it->currentY);
      }
      snowflakes.clear();
      bool snowLeft = false;
      for (int x = 0; x < WIDTH; x++) {
        int xx = meltDistributor.getNextCoord();
        if (snowLevel[xx] < HEIGHT) {
          bitmap->clearBit(xx, snowLevel[xx]++);
          delay(rand() % 1000);
          snowLeft = true;
        }
      }
      if (! snowLeft) {
        restart();
      }
    }
    void snow(unsigned long now) {
      for (std::vector<Snowflake>::iterator it = snowflakes.begin(); it != snowflakes.end(); ++it) {
        if (now > it->lastRedraw + it->velocityInMS) {
          if (it->currentY > -1) {
            bitmap->clearBit(it->currentX, it->currentY);
          }
          it->currentY++;
          if (it->currentY > bitmap->height - 1) {
            it->currentY = -1;
            it->velocityInMS = rand() % (maxVelocity - minVelocity) + minVelocity;
          }
          bitmap->setBit(it->currentX, it->currentY);
          it->lastRedraw = now;
        }
      }
      if (now > lastAddedTime + 1000) {
        if (snowflakes.size() < WIDTH) {
            snowflakes.push_back(createSnowflake());
            lastAddedTime = now;
        }
      }
    }
    void display() {
      if (show) {
        unsigned long now = millis();
        if (snowing) {
          snow(now);
        } else {
          melt(now);
        }
        LEDStripWrapper::showBitmap(bitmap);
      }
    }
    void runTest(String title, bool showOLED, bool showLEDStrip, bool showTextBitmap) {
      if (showOLED) {
        oledWrapper->bitmap(bitmap);
        delay(2000);
      }
      if (showLEDStrip) {
        LEDStripWrapper::showBitmap(bitmap);
        delay(5000);
        LEDStripWrapper::clear();
      }
      if (showTextBitmap) {
        bitmap->dump(title);
      }
      bitmap->clear();
    }
    void bitmapTest(bool showOLED, bool showLEDStrip, bool showTextBitmap) {
      bitmap->createDiagonals();
      runTest("diagonals", showOLED, showLEDStrip, showTextBitmap);
      bitmap->fill();
      runTest("fill", showOLED, showLEDStrip, showTextBitmap);
    }
};
XmasDisplayer xmasDisplayer;

class App {
  private:
    bool  showOLED = false;
    bool  showLEDStrip = true;
    bool  showTextBitmap = false;

    String cmds = "runSpeedTest, "
                    "runRampTest, runBitmapTest, "
                    "showOLED, showLEDStrip, showTextBitmap, "
                    "hideOLED, hideLEDStrip, hideTextBitmap, "
                    "clear, theDelay=[delayInMilliseconds], "
                    "showBuild, capacityTest";
    String configs[2] = {
      "~2024Dec24:09:11", // date +"%Y%b%d:%H:%M"
      "https://github.com/chrisxkeith/xmas-led",
    };

    void showBuild() {
      oledWrapper->clear();
      oledWrapper->addText(0, 0, configs[0]);
      oledWrapper->endDisplay();
    }
    void oledPanelTest() {
      Bitmap* b = new Bitmap(kOLED1in3Width, kOLED1in3Height);
      b->createDiagonals();
      oledWrapper->bitmap(b);
      delay(3000);
      b->fill();
      oledWrapper->bitmap(b);
    }
    void checkSerial() {
      if (Serial.available() > 0) {
        String teststr = Serial.readString();  // read until timeout
        teststr.trim();                        // remove any \r \n whitespace at the end of the String
        if (teststr.equals("runSpeedTest")) {
          LEDStripWrapper::speedTest();
        } else if (teststr.equals("runRampTest")) {
          LEDStripWrapper::rampTestForward();
          LEDStripWrapper::rampTestBackward();
        } else if (teststr.equals("runBitmapTest")) {
          xmasDisplayer.bitmapTest(showOLED, showLEDStrip, showTextBitmap);
        } else if (teststr.equals("clear")) {
          LEDStripWrapper::clear();
          oledWrapper->clear();
          xmasDisplayer.clear();
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
        } else if (teststr.startsWith("capacityTest")) {
          LEDStripWrapper::capacityTest();
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
      LEDStripWrapper::startup();
      Utils::scanI2C();
      oledWrapper = new OLEDWrapper(false);
      oledWrapper->clear();
      Serial.println(cmds.c_str());
    }
    void loop() {
      xmasDisplayer.display();
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
