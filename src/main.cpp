#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_TinyUSB.h>
#include <WiiChuck.h>
#include <Adafruit_IS31FL3741.h>
#include <Adafruit_Debounce.h>

boolean DEBUG = false;

TwoWire *i2c = &Wire;
Adafruit_NeoPixel neopixel_status(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel neopixel_mode(1, PIN_A3, NEO_GRB + NEO_KHZ800);
Adafruit_Debounce button_mode(PIN_A2, LOW);
Adafruit_IS31FL3741_QT ledmatrix;
Accessory acc;

auto RED = Adafruit_NeoPixel::gamma32(0xFF0000);
auto ORANGE = Adafruit_NeoPixel::gamma32(0xFF8800);
auto YELLOW = Adafruit_NeoPixel::gamma32(0xFFFF00);
auto GREEN = Adafruit_NeoPixel::gamma32(0x00FF00);
auto BLUE = Adafruit_NeoPixel::gamma32(0x0000FF);
auto INDIGO = Adafruit_NeoPixel::gamma32(0x8800FF);
auto PURPLE = Adafruit_NeoPixel::gamma32(0xFF00FF);
auto WHITE = Adafruit_NeoPixel::gamma32(0xFFFFFF);
auto BLACK = Adafruit_NeoPixel::gamma32(0x000000);

int MODE_L_STICK = 0;
int MODE_D_PAD = 1;
int MODE_MOUSE = 2;
int MODE_COUNT = 3;
uint32_t MODE_COLORS[] = {
  // Blue for Left-stick
  BLUE,
  // Green for Gamepad (D-pad)
  GREEN,
  // Magenta/Purple for Mouse
  PURPLE,
};
String MODE_NAMES[] = {
  "L-Stick",
  "D-Pad",
  "Mouse",
};
int mode = MODE_L_STICK;

bool isSupportedAccessory = false;

int IS31_ADDRESS = 0x30;
boolean is31_found = false;

// forward declarations so I can organize how I want to
void check_wii_accessory();
void process_nunchuck();
void process_classic();

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("Starting");

  Serial.println("Set up Little NeoPixel (board side) to show status");
  neopixel_status.begin();
  neopixel_status.setBrightness(5);
  neopixel_status.fill(YELLOW);
  neopixel_status.show();

  Serial.println("Set up Big NeoPixel (button side) to show mode");
  neopixel_mode.begin();
  neopixel_mode.setBrightness(5);
  neopixel_mode.setPixelColor(0, MODE_COLORS[mode]);
  neopixel_mode.show();

  Serial.println("Set up Big Button to change modes");
  button_mode.begin();

  if (ledmatrix.begin(IS31_ADDRESS, i2c)) {
    Serial.printf("IS41 found at 0x%X\n", IS31_ADDRESS);
    is31_found = true;
    i2c->setClock(800000L);
    ledmatrix.setLEDscaling(0xFF);
    ledmatrix.setGlobalCurrent(0x10);
    ledmatrix.enable(true);
    ledmatrix.drawPixel(6, 4, MODE_COLORS[mode]);
    ledmatrix.show();
  } else {
    Serial.printf("IS41 not found at 0x%X\n", IS31_ADDRESS);
  }

  check_wii_accessory();
  if (acc.isConnected()) {
    neopixel_status.fill(GREEN);
    neopixel_status.show();
  }
}

int jX = 0;
int jY = 0;
boolean bZ = false;
boolean bC = false;

double theta;
double degrees;
int hue;
int val;
uint32_t color;
int16_t x_pos;
int16_t y_pos;
int16_t x_pos_old;
int16_t y_pos_old;

void loop() {
  button_mode.update();
  if (button_mode.justPressed()) {
    mode += 1;
    if (mode >= MODE_COUNT) {
      mode = 0;
    }
    Serial.print("Changing mode to ");
    Serial.println(MODE_NAMES[mode]);
  }
  neopixel_mode.setPixelColor(0, MODE_COLORS[mode]);
  neopixel_mode.show();

  if (!acc.isConnected()) {
    Serial.println("Nothing connected, trying again in 2 seconds.");
    neopixel_status.setPixelColor(0, RED);
    neopixel_status.show();
    delay(2000);
    acc.reset();
    check_wii_accessory();
    return;
  }

  if (!acc.readData()) {
    Serial.println("Could not read data from Wii Accessory");
    neopixel_status.setPixelColor(0, RED);
    neopixel_status.show();
    return;
  }
  neopixel_status.setPixelColor(0, GREEN);
  neopixel_status.show();

  if (acc.type == NUNCHUCK) {
    jX = acc.getJoyX() - 128;
    jY = acc.getJoyY() - 128;
    const int aX = acc.getAccelX();
    const int aY = acc.getAccelY();
    const int aZ = acc.getAccelZ();
    bZ = acc.getButtonZ();
    bC = acc.getButtonC();
    if (DEBUG) {
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
    }
  } else if (acc.type == WIICLASSIC) {
    process_classic();
  }

  if (is31_found) {
    theta = atan2(jX, jY);
    degrees = theta * (180.0 / M_PI);
    degrees += degrees < 0 ? 360 : 0;
    hue = static_cast<int>(degrees) / 360.0 * 65535;
    val = static_cast<int>(sqrt(pow(jX, 2) + pow(jY, 2)));
    if (val == 0) {
      // use dim white in the middle
      color = Adafruit_NeoPixel::gamma32(
        Adafruit_IS31FL3741_QT::ColorHSV(hue, 0, 128)
      );
    } else {
      color = Adafruit_NeoPixel::gamma32(
        Adafruit_IS31FL3741_QT::ColorHSV(hue, 255, 255)
      );
    }
    const uint16_t matrix_color_565 = Adafruit_IS31FL3741_QT::color565(color);

    const auto x = static_cast<int>(12.0 * (jX - 127.0) / (255.0) + 6.0);
    const auto y = static_cast<int>(8.0 * (jY - 127.0) / (255.0) + 4.0);
    x_pos = 6 + x;
    y_pos = 4 - y;
    if (x_pos_old != x_pos || y_pos_old != y_pos) {
      // clear the old pixel
      ledmatrix.drawPixel(x_pos_old, y_pos_old, BLACK);
      // save the new pixel
      x_pos_old = x_pos;
      y_pos_old = y_pos;
    }
    ledmatrix.drawPixel(x_pos, y_pos, matrix_color_565);
    ledmatrix.show();
  }
}

void check_wii_accessory() {
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
