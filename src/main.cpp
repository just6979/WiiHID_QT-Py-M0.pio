#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_TinyUSB.h>
#include <WiiChuck.h>
#include <Adafruit_IS31FL3741.h>

TwoWire *i2c = &Wire;
Adafruit_NeoPixel neo_small(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel neo_big(1, PIN_A3, NEO_GRB + NEO_KHZ800);
Adafruit_IS31FL3741_QT ledmatrix;
Accessory acc;

uint8_t IS31_ADDR = 0x30;
bool isSupportedAccessory = false;

// forward declarations so I can organize how I want to
void check_accessory();
void process_nunchuck();
void process_classic();

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }
  Serial.println("Starting");

  neo_small.begin();
  neo_small.setBrightness(10);
  neo_big.begin();
  neo_big.setBrightness(10);
  neo_small.setPixelColor(0, 0x00FF00);
  neo_big.setPixelColor(0, 0x00FF00);
  neo_small.show();
  neo_big.show();

  check_accessory();

  if (ledmatrix.begin(IS31_ADDR, i2c)) {
    Serial.printf("IS41 found at 0x%X\n", IS31_ADDR);
  } else {
    Serial.printf("IS41 not found at 0x%X\n", IS31_ADDR);
  }
  i2c->setClock(800000L);
  ledmatrix.setLEDscaling(0xFF);
  ledmatrix.setGlobalCurrent(0x10);
  Serial.printf("Global current set to: ");
  Serial.println(ledmatrix.getGlobalCurrent());
  ledmatrix.enable(true);
}

void loop() {
  if (!acc.isConnected()) {
    Serial.println("Nothing connected, trying again in 2 seconds.");
    delay(2000);
    acc.reset();
    check_accessory();
    return;
  }

  neo_big.clear();
  neo_small.clear();

  acc.readData();

  int jX = 0;
  int jY = 0;
  boolean bZ = false;
  boolean bC = false;
  if (acc.type == NUNCHUCK) {
    jX = acc.getJoyX() - 128;
    jY = acc.getJoyY() - 128;
    const int aX = acc.getAccelX();
    const int aY = acc.getAccelY();
    const int aZ = acc.getAccelZ();
    bZ = acc.getButtonZ();
    bC = acc.getButtonC();
    Serial.printf(
      "J[%4d,%4d];A[%3d,%3d,%3d];B[%s%s]\n",
      jX,
      jY,
      aX,
      aY,
      aZ,
      bZ ? "Z" : " ",
      bC ? "C" : " "
    );
  } else if (acc.type == WIICLASSIC) {
    process_classic();
  }

  const auto color_x = jX;
  const auto color_y = jY;
  const int val = static_cast<int>(sqrt(pow(color_x, 2) + pow(color_y, 2)));
  const double t = atan2(color_y, color_x);
  double d = t * (180.0 / M_PI);
  d += d < 0 ? 360 : 0;
  const int hue = static_cast<int>(d) / 360.0 * 65535;

  uint32_t color;
  if (val != 0) {
    color = Adafruit_IS31FL3741_QT::ColorHSV(hue, 255, 255);
  } else {
    // matrix_color = 0xFFFFFF;
    color = Adafruit_IS31FL3741_QT::ColorHSV(hue, 0, 128);
  }

  if (bC) {
    neo_small.fill(color);
  } else {
    neo_small.clear();
  }
  if (bZ) {
    neo_big.fill(color);
  } else {
    neo_big.clear();
  }
  neo_small.show();
  neo_big.show();

  const auto x = (12.0 * (jX - 127.0) / (255.0) + 6.0);
  const auto y = (8.0 * (jY - 127.0) / (255.0) + 4.0);
  ledmatrix.fill(0x000000);
  ledmatrix.drawFastHLine(0, 16, 2, Adafruit_IS31FL3741_QT::color565(0xFF0000));
  const uint16_t matrix_color_565 = Adafruit_IS31FL3741_QT::color565(
    color
  );
  ledmatrix.drawPixel(6 + x, 4 - y, matrix_color_565);
}

void check_accessory() {
  acc.begin();
  Serial.printf("Accessory Type: %d\n", acc.type);
  switch (acc.type) {
    case NUNCHUCK:
      Serial.println("Found Nunchuck");
      isSupportedAccessory = true;
      break;
    case WIICLASSIC:
      Serial.println("Found Wii Classic Controller");
      isSupportedAccessory = true;
      break;
    case GuitarHeroController:
      Serial.println("Found Guitar Hero Controller");
      break;
    case GuitarHeroWorldTourDrums:
      Serial.println("Found Guitar Hero World TourDrums");
      break;
    case DrumController:
      Serial.println("Found Drum Controller");
      break;
    case DrawsomeTablet:
      Serial.println("Found Drawsome Tablet");
      break;
    case Turntable:
      Serial.println("Found Turntable");
      break;
    case UnknownChuck:
      Serial.println("Unknown accessory type");
      break;
  }
  if (!isSupportedAccessory) {
    Serial.println(
      "We don't support this accessory type [yet?] for HID, "
      "so we'll just treat it as a Nunchuck."
    );
  }
}

void process_nunchuck() {
  const int jX = acc.getJoyX();
  const int jY = acc.getJoyY();
  const int aX = acc.getAccelX();
  const int aY = acc.getAccelY();
  const int aZ = acc.getAccelZ();
  const boolean bZ = acc.getButtonZ();
  const boolean bC = acc.getButtonC();

  Serial.printf(
    "J[%03d,%03d];A[%03d,%03d,%03d];B[%s%s]\n",
    jX,
    jY,
    aX,
    aY,
    aZ,
    bZ ? "Z" : " ",
    bC ? "C" : " "
  );
}

void process_classic() {
  const int lX = acc.getJoyXLeft();
  const int lY = acc.getJoyYLeft();
  const int rX = acc.getJoyXRight();
  const int rY = acc.getJoyYRight();
  const int tL = acc.getTriggerLeft();
  const int tR = acc.getTriggerRight();
  const boolean bA = acc.getButtonA();
  const boolean bB = acc.getButtonB();
  const boolean bX = acc.getButtonX();
  const boolean bY = acc.getButtonY();
  const boolean bM = acc.getButtonMinus();
  const boolean bP = acc.getButtonPlus();
  const boolean bH = acc.getButtonHome();
  const boolean bZL = acc.getButtonZLeft();
  const boolean bZR = acc.getButtonZRight();

  Serial.printf(
    "JL[%03d,%03d];JR[%03d,%03d];T[L%03d,R%03d]\n"
    "B[%s%s%s%s][%s%s][%s%s%s]\n",
    lX,
    lY,
    rX,
    rY,
    tL,
    tR,
    bA ? "A" : " ",
    bB ? "B" : " ",
    bX ? "X" : " ",
    bY ? "Y" : " ",
    bZL ? "ZL" : "  ",
    bZR ? "ZR" : " ",
    bM ? "-" : " ",
    bH ? "H" : " ",
    bP ? "P" : " "
  );
}
