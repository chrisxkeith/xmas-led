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
  private:
    static int lastRand;
    const static int RAND_INCREMENT = 37;
  public:
    static bool diagnosing;
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
    static int myRand() {
      int ret = lastRand + RAND_INCREMENT;
      if (ret < 0) {
        ret = 0;
      }
      lastRand = ret;
      return ret;
    }
    static void resetRand() {
      lastRand = 0;
    }
};
int Utils::lastRand = 0;
bool Utils::diagnosing = true;

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
    int bitCount() {
      int ret = 0;
      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
          if (getBit(x, y)) {
            ret++;
          }
        }
      }
      return ret;
    }
    int sizeInBytes() { return width * height / 8; }
    int calcByteIndex(int x, int y) {
      if (x < 0 || y < 0) {
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
        return 0;
      }
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
    void dumpForPixel(int x, int y, String title) {
      if (x == 9 && y == 12) {
        String msg(title);
        msg.concat(" (");
        msg.concat(x);
        msg.concat(", ");
        msg.concat(y);
        msg.concat(") ");
        Serial.println(msg);
      }
    }
    void setBit(int x,  int y) {
      int i = calcByteIndex(x, y);
      bitmap[i] |= (1 << (7 - (x % 8)));
      dumpForPixel(x, y, "setBit");
    }
    void clearBit(int x,  int y) {
      int i = calcByteIndex(x, y);
      bitmap[i] &= ~(1 << (7 - (x % 8)));
      dumpForPixel(x, y, "clearBit");
    }
    int getBit(int x, int y) {
      byte b = bitmap[calcByteIndex(x, y)];
      bool bit = b >> (7 - (x % 8)) & 0x1;
      return bit;
    }
    void dump(String title) {
      dump(title, 0, height);
    }    
    void dump(String title, int topRow, int afterBottomRow) {
      if (!title.equals("")) {
        Serial.println(title);
      }
      String msg;
      for (int y = topRow; y < afterBottomRow; y++) {
        msg.remove(0);
        msg.concat(y < 10 ? " " : "");
        msg.concat(y);
        msg.concat(" ");
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
    static const int  DATA_PIN = 8;   // green wire
    static const int  CLOCK_PIN = 10; // blue wire
    static uint32_t   theDelay;
    static void speedTest() {
      // Timer timer("speedTest()");
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
      // speedTest();
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
    int                   calledCount = 0;
    bool                  doReset;
  public:
    RandomDistributor(bool doReset = false) {
      this->doReset = doReset;
    }
    void reset() {
      xcoords.reset();
      calledCount = 0;
    }
    int getNextCoord() {
      if (calledCount >= N_COORDS) {
        if (doReset) {
          reset();
        } else {
          String err("RandomDistributor: called too many times");
          return Utils::myRand() % N_COORDS;
        }
      }
      int x = Utils::myRand() % N_COORDS;
      while (xcoords[x]) {
        x = Utils::myRand() % N_COORDS;
      }
      xcoords[x] = true;
      calledCount++;
      return x;
    }
};

class XmasDisplayer {
  private:
    const static int   WIDTH = 16;
    const static int   HEIGHT = 16;

    Bitmap*            bitmap;
    vector<Snowflake>  snowflakes;
    int                endFlakeCount = 16;
    unsigned long      lastAddedTime = 0;
    unsigned long      lastMeltTime = 0;
    RandomDistributor  flakeDistributor;
    RandomDistributor  meltDistributor = RandomDistributor(true);
    int                snowLevel[WIDTH];
    enum SnowState {
        snowing,
        stopping,
        melting
    };
    SnowState          snowState = snowing;
    unsigned long      lastRestart = 0;
    const int          BETWEEN_STATE_WAIT = 2000;
    const int          MAX_SNOW_HEIGHT_COORD = HEIGHT - 3;  // 3 solid rows of snow on the ground, with some higher drifts

    int calculateVelocity() {
      return Utils::myRand() % (maxVelocity - minVelocity) + minVelocity;
    }
    Snowflake createSnowflake() {
      return Snowflake(flakeDistributor.getNextCoord(), -1,  calculateVelocity());
    }
    String snowStateName(SnowState ss) {
      switch (ss) {
        case snowing:  return "snowing";
        case stopping: return "stopping";
        case melting:  return "melting";
        default:       return "Unknown SnowState!";
      }
    }
    void changeState(SnowState ss) {
      String s("Changing state from: ");
      s.concat(snowStateName(snowState));
      s.concat(" to: ");
      s.concat(snowStateName(ss));
      // Serial.println(s);
      snowState = ss;
    }
    void checkState(String s) {
      if (snowflakes.size() > 0) {
        String err(s);
        err.concat(": snowflakes left: ");
        err.concat(snowflakes.size());
        snowflakes.clear();
      }
      if (bitmap->bitCount() > 0) {
        String err(s);
        err.concat(": bits left: ");
        err.concat(bitmap->bitCount());
        bitmap->clear();
      }
      String e2;
      for (int i = 0; i < WIDTH; i++) {
        if (snowLevel[i] < HEIGHT) {
          e2.concat(i);
          e2.concat(" ");
        }
        if (e2.length() > 0) {
          String ee("Non-melted snow: ");
          ee.concat(e2);
          Serial.println(e2);
        }
      }
    }
    void doPause(String msg) {
      const int PAUSE_SECONDS = 0;

      if (PAUSE_SECONDS > 0) {          
        String s(msg);
        s.concat(": Delaying... ");
        Serial.print(s);
        for (int i = PAUSE_SECONDS; i > 0; i--) {
          String msg(i);
          msg.concat(" ");
          Serial.print(msg);
          delay(1000);        
        }
        Serial.println("");
      }
    }
    void restart() {
      if (lastRestart > 0 && !Utils::diagnosing) {
        unsigned long seconds = (millis() - lastRestart) / 1000;
        String s("cycle time in seconds: ");
        s.concat(seconds);
        Serial.println(s);
      }
      start(false);
      checkState("restart()");
      changeState(snowing);
    }
  public:
    bool               show = true;
    long               minVelocity = 100;
    int                maxVelocity = 500;
    
    XmasDisplayer() {
      bitmap = new Bitmap(WIDTH, HEIGHT);
      start(true);
    }
    void start(bool constructing) {
      lastRestart = millis();
      flakeDistributor.reset();
      meltDistributor.reset();
      Utils::resetRand();
      for (int i = 0; i < WIDTH; i++) {
        snowLevel[i] = HEIGHT;
      }
      LEDStripWrapper::clear();
      oledWrapper->clear();
      bitmap->clear();
      show = true;
    }
    void clear() {
      if (show) {
        LEDStripWrapper::clear();
      }
      show = ! show;
    }
    void melt(unsigned long now) {
      if (now > lastMeltTime + 100) {
        int xx = meltDistributor.getNextCoord();
        if (snowLevel[xx] < HEIGHT) {
          bitmap->clearBit(xx, snowLevel[xx]++);
          lastMeltTime = millis();
        }
      }
      bool snowLeft = false;
      for (int x = 0; x < WIDTH; x++) {
        if (snowLevel[x] < HEIGHT) {
          snowLeft = true;
          break;  
        }      
      }
      if (! snowLeft) {
        delay(BETWEEN_STATE_WAIT);
        restart();
      }
    }
    void dumpForSnowflake(String m) {
      m.concat(": Dumping for snowflake at (8,12), snowState: ");
      m.concat(snowStateName(snowState));
      Serial.println(m);
      String s("Active Snowflakes: ");
    }
    void snow(unsigned long now) {
      for (std::vector<Snowflake>::iterator it = snowflakes.begin(); it != snowflakes.end(); ++it) {
        if (now > it->lastRedraw + it->velocityInMS) {
          if (it->currentY > -1 && it->currentY < snowLevel[it->currentX] - 1) {
            bitmap->clearBit(it->currentX, it->currentY);
            it->lastRedraw = now;
          }
          if (snowState == stopping) {
            it = snowflakes.erase(it);
            if (it == snowflakes.end()) {
              break;
            }
          } else {
            it->currentY++;
            if (it->currentY > snowLevel[it->currentX] - 1) {
              if (snowLevel[it->currentX] < MAX_SNOW_HEIGHT_COORD) {
                continue;
              }
              snowLevel[it->currentX]--;
              it->currentY = -1;
              it->velocityInMS = calculateVelocity();
            }
            if (it->currentY >= 0) {
              bitmap->setBit(it->currentX, it->currentY);
              it->lastRedraw = now;
            }
          }
        }
      }
      if (snowState == snowing) {
        //  Look at snowLevel to see if we're all done snowing?
        SnowState nextState = stopping;
        for (int i = 0; i < WIDTH; i++) {
          if (snowLevel[i] > MAX_SNOW_HEIGHT_COORD) {
            nextState = snowing;
            break;
          }
        }
        if (nextState == stopping) {
          changeState(stopping);
          return;
        }
        if (now > lastAddedTime + 100) {
          if (snowflakes.size() < WIDTH) {
              snowflakes.push_back(createSnowflake());
              lastAddedTime = now;
          }
        }
      }
    }
    bool display(bool showOLED) {
      SnowState previousState = snowState;
      if (show) {
        unsigned long now = millis();
        switch (snowState) {
          case snowing:
            snow(now);
            break;
          case stopping:
            snow(now);
            if (snowflakes.size() == 0) {
              changeState(melting);
              dump();
              delay(BETWEEN_STATE_WAIT);
              lastMeltTime = millis();
            }
            break;
          case melting:
            melt(now);
            break;
        }
        LEDStripWrapper::showBitmap(bitmap);
        if (showOLED) {
          oledWrapper->bitmap(bitmap);
        }
      }
      return (previousState == stopping && snowState == melting);
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
    void dump() {
      String s("snowLevel: ");
      int snowLevelsAt12 = 0;
      for (int i = 0; i < WIDTH; i++) {
        s.concat(snowLevel[i]);
        s.concat(" ");
        if (snowLevel[i] == 12) {
          snowLevelsAt12++;
        }
      }
      size_t pixelsAt12 = 0;
      for (size_t i = 0; i < WIDTH; i++) {
        if (bitmap->getBit(i, 12)) {
          pixelsAt12++;
        }
      }
      if (snowLevelsAt12 != pixelsAt12) {
        String err("Mismatch: snowLevelsAt12: ");
        err.concat(snowLevelsAt12);
        err.concat(", pixelsAt12: ");
        err.concat(pixelsAt12);
        err.concat(", maxVelocity: ");
        err.concat(maxVelocity);
        err.concat(", minVelocity: ");
        err.concat(minVelocity);
        Serial.println(err);
        Serial.println(s);
        bitmap->dump("", 12, 13);
      }
    }
};
XmasDisplayer xmasDisplayer;

class App {
  private:
    bool  showOLED = Utils::diagnosing;
    bool  showLEDStrip = true;
    bool  showTextBitmap = false;

    String cmds = "runSpeedTest, "
                    "runRampTest, runBitmapTest, "
                    "showOLED, showLEDStrip, showTextBitmap, "
                    "hideOLED, hideLEDStrip, hideTextBitmap, "
                    "clear, theDelay=[delayInMilliseconds], "
                    "showBuild, capacityTest, start, stop, "
                    "dump, continue";
    String configs[2] = {
      "~2025Dec05:11:15", // date +"%Y%b%d:%H:%M"
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
    void incrementVelocities() {
      xmasDisplayer.maxVelocity += 20;
      xmasDisplayer.minVelocity += 20;
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
        } else if (teststr.startsWith("start")) {
          xmasDisplayer.show = true;
          xmasDisplayer.start(true);
        } else if (teststr.startsWith("dump")) {
          xmasDisplayer.dump();
        } else if (teststr.startsWith("stop")) {
          LEDStripWrapper::clear();
          oledWrapper->clear();
          xmasDisplayer.clear();
          xmasDisplayer.show = false;
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
      // Utils::scanI2C();
      oledWrapper = new OLEDWrapper(false);
      oledWrapper->clear();
      if (Utils::diagnosing) {
        xmasDisplayer.maxVelocity = 100;
        xmasDisplayer.minVelocity = 50;
      }
    }
    void loop() {
      if (Utils::diagnosing) {
        if (xmasDisplayer.display(showOLED)) {
          incrementVelocities();
          if (xmasDisplayer.maxVelocity > 100) {
            Serial.println("... Stopping.");
            while (true) { ; }
          } 
        }
      } else {
        xmasDisplayer.display(showOLED);
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
