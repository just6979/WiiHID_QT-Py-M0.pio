#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_TinyUSB.h>
#include <WiiChuck.h>
#include <Adafruit_IS31FL3741.h>
#include <Adafruit_Debounce.h>

const auto RED = Adafruit_NeoPixel::gamma32(0xFF0000);
const auto ORANGE = Adafruit_NeoPixel::gamma32(0xFF8800);
const auto YELLOW = Adafruit_NeoPixel::gamma32(0xFFFF00);
const auto GREEN = Adafruit_NeoPixel::gamma32(0x00FF00);
const auto LIGHT_BLUE = Adafruit_NeoPixel::gamma32(0x87CEFA);
const auto BLUE = Adafruit_NeoPixel::gamma32(0x0000FF);
const auto DARK_BLUE = Adafruit_NeoPixel::gamma32(0x0000AA);
const auto INDIGO = Adafruit_NeoPixel::gamma32(0x8800FF);
const auto PURPLE = Adafruit_NeoPixel::gamma32(0xFF00FF);
const auto WHITE = Adafruit_NeoPixel::gamma32(0xFFFFFF);
const auto GRAY = Adafruit_NeoPixel::gamma32(0x888888);
const auto BLACK = Adafruit_NeoPixel::gamma32(0x000000);

Adafruit_NeoPixel neopixel_status(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel neopixel_mode(1, PIN_A3, NEO_GRB + NEO_KHZ800);
Adafruit_Debounce button_mode(PIN_A2, LOW);

constexpr uint8_t NEOPIXEL_STATUS_BRIGHTNESS = 1;
constexpr uint8_t NEOPIXEL_MODE_BRIGHTNESS = 5;

constexpr uint8_t MODE_L_STICK = 0;
constexpr uint8_t MODE_D_PAD = 1;
constexpr uint8_t MODE_MOUSE = 2;
constexpr uint8_t MODE_GAME = 3;
constexpr uint8_t MODE_COUNT = 4;
const String MODE_NAMES[MODE_COUNT] = {
  "L-Stick",
  "D-Pad",
  "Mouse",
  "Game"
};
const uint32_t MODE_COLORS[MODE_COUNT] = {
  // Left-stick
  LIGHT_BLUE,
  // D-pad
  DARK_BLUE,
  // "Magenta" for Mouse
  PURPLE,
  // Game
  GREEN

};
// set initial mode minus 1 because next_mode() is called in setup()
uint8_t mode = MODE_GAME - 1;

TwoWire *i2c = &Wire;

Accessory acc;
bool nunchuck_found = false;
uint8_t NUNCHUCK_ADDRESS = 0x52;
int8_t jX = 0;
int8_t jY = 0;
bool bZ = false;
bool bC = false;

constexpr uint8_t desc_hid_report[] = {
  TUD_HID_REPORT_DESC_GAMEPAD()
};
Adafruit_USBD_HID usb_hid;
hid_gamepad_report_t gp;

Adafruit_IS31FL3741_QT ledmatrix;
bool is31_found = false;
constexpr uint8_t IS31_ADDRESS = 0x30;
constexpr uint8_t IS31_LED_SCALING = 0xFF;
constexpr uint8_t IS31_GLOBAL_CURRENT = 0x01;

const auto JOY_COLOR = Adafruit_IS31FL3741_QT::color565(RED);
const auto Z_COLOR = Adafruit_IS31FL3741_QT::color565(PURPLE);
const auto C_COLOR = Adafruit_IS31FL3741_QT::color565(ORANGE);
uint32_t z_fill_color;
uint32_t c_fill_color;

double theta;
double degrees;
int32_t val;
int8_t x_pos;
int8_t y_pos;
int8_t x_pos_old;
int8_t y_pos_old;

ulong now;
constexpr auto WII_UPDATE_DELAY = 2; // 500 Hz
ulong last_wii_update = 0;
constexpr auto HID_UPDATE_DELAY = 8; // 125 Hz
ulong last_hid_update = 0;
constexpr auto LED_UPDATE_DELAY = 16; // 60 Hz
ulong last_led_update = 0;

void next_mode();
void check_wii_acc();
bool update_wii_acc();
void update_usb_hid();
void is31_show_nunchuck();
void update_game();


void setup() {
  Serial.begin(115200);

#if DEBUG
  auto start = millis();
  while (!Serial) {
    if (millis() - start > 2000) continue;
  }
#endif

  Serial.println("Starting");

  Serial.println("Set up Little NeoPixel (board side) to show status");
  neopixel_status.begin();
  neopixel_status.setBrightness(NEOPIXEL_STATUS_BRIGHTNESS);
  neopixel_status.fill(YELLOW);
  neopixel_status.show();

  Serial.println("Set up TinyUSB HID");
  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.begin();
  // reattach to ensure all drivers start
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
    neopixel_status.fill(YELLOW);
    neopixel_status.show();
  }

  check_wii_acc();

  if (ledmatrix.begin(IS31_ADDRESS, i2c)) {
    Serial.printf("IS41 found at 0x%X\n", IS31_ADDRESS);
    is31_found = true;
    ledmatrix.setLEDscaling(IS31_LED_SCALING);
    ledmatrix.setGlobalCurrent(IS31_GLOBAL_CURRENT);
    ledmatrix.enable(true);
    ledmatrix.fill(BLACK);
    ledmatrix.show();
  } else {
    Serial.printf("IS41 not found at 0x%X\n", IS31_ADDRESS);
  }

  Serial.println("Set up Big NeoPixel (button side) to show mode");
  neopixel_mode.begin();
  neopixel_mode.setBrightness(NEOPIXEL_MODE_BRIGHTNESS);

  Serial.println("Set up Big Button to change modes");
  button_mode.begin();
  next_mode();

  Serial.println("Done setup");
}


void loop() {
  now = millis();

  button_mode.update();
  if (button_mode.justReleased()) {
    next_mode();
  }

  if (!TinyUSBDevice.mounted()) {
    Serial.println("USB HID not mounted, trying again");
    TinyUSBDevice.attach();
    neopixel_status.setPixelColor(0, RED);
    return;
  }

  if (now - last_wii_update >= WII_UPDATE_DELAY) {
    update_wii_acc();
    last_wii_update = now;
  }

  if (now - last_hid_update >= HID_UPDATE_DELAY) {
    if (usb_hid.ready()) {
      update_usb_hid();
    }
    last_hid_update = now;
  }

  if (is31_found) {
    if (now - last_led_update >= LED_UPDATE_DELAY) {
      if (mode != MODE_GAME) {
        is31_show_nunchuck();
      } else {
        update_game();
      }
      last_led_update = now;
    }
  }
}

void next_mode() {
  mode++;
  // TODO: remove this skip when mouse mode is implemented
  if (mode == MODE_MOUSE) {
    mode++;
  }
  if (mode == MODE_GAME && !is31_found) {
    Serial.print("Can't play game without LED matrix plugged in.");
    mode++;
  }
  if (mode >= MODE_COUNT) {
    mode = 0;
  }
  Serial.print("Changing mode to ");
  Serial.println(MODE_NAMES[mode]);
  neopixel_mode.setPixelColor(0, MODE_COLORS[mode]);
  neopixel_mode.show();
}

void check_wii_acc() {
  Serial.println("Checking for Wii accesorries");
  acc.begin();
  if (acc.type == NUNCHUCK) {
    Serial.println("Found Nunchuck");
    nunchuck_found = true;
    neopixel_status.fill(GREEN);
    neopixel_status.show();
  } else {
    Serial.printf("Missing or unknown accessory (type:%d)", acc.type);
    Serial.println();
    nunchuck_found = false;
    neopixel_status.fill(YELLOW);
    neopixel_status.show();
  }
}

bool update_wii_acc() {
  if (!acc.isConnected()) {
    Serial.println(
      "No known Wii accessory connected, trying again in 2 seconds."
    );
    neopixel_status.setPixelColor(0, YELLOW);
    neopixel_status.show();
    delay(2000);
    acc.reset();
    check_wii_acc();
  }
  if (!acc.isConnected()) {
    return false;
  }

  if (!acc.readData()) {
    Serial.println("Could not read data from Wii Accessory, trying again");
    neopixel_status.setPixelColor(0, YELLOW);
    neopixel_status.show();
    return false;
  }

  if (acc.type == NUNCHUCK) {
    neopixel_status.setPixelColor(0, GREEN);
    neopixel_status.show();
    jX = static_cast<int8_t>(acc.getJoyX() - 128);
    jY = static_cast<int8_t>(acc.getJoyY() - 128);
    if (jX < -127) { jX = -127; }
    if (jY < -127) { jY = -127; }
    bZ = acc.getButtonZ();
    bC = acc.getButtonC();
#if DEBUG
    const int aX = acc.getAccelX();
    const int aY = acc.getAccelY();
    const int aZ = acc.getAccelZ();
    Serial.printf(
      "J[%4d,%4d];A[%3d,%3d,%3d];B[%s%s]",
      jX,
      jY,
      aX,
      aY,
      aZ,
      bZ ? "Z" : " ",
      bC ? "C" : " "
    );
    Serial.println();
#endif
  }
  return true;
}

void update_usb_hid() {
  // TODO: don't repeat sending identical reports
  // reset
  gp.x = 0;
  gp.y = 0;
  gp.hat = 0;
  gp.buttons = 0;

  if (mode == MODE_L_STICK) {
    gp.x = jX;
    // flip Y axis, not sure why
    gp.y = static_cast<int8_t>(jY * -1);
  }

  if (mode == MODE_D_PAD) {
    theta = atan2(jY, jX);
    degrees = theta * (180.0 / M_PI);
    degrees += degrees < 0 ? 360 : 0;
    val = static_cast<int>(sqrt(pow(jX, 2) + pow(jY, 2)));
    if (val > 64) {
      if (degrees < 22.5 || degrees > 337.5) gp.hat = GAMEPAD_HAT_RIGHT;
      if (degrees < 67.5 && degrees > 22.5) gp.hat = GAMEPAD_HAT_UP_RIGHT;
      if (degrees < 112.5 && degrees > 67.5) gp.hat = GAMEPAD_HAT_UP;
      if (degrees < 157.5 && degrees > 112.5) gp.hat = GAMEPAD_HAT_UP_LEFT;
      if (degrees < 202.5 && degrees > 157.5) gp.hat = GAMEPAD_HAT_LEFT;
      if (degrees < 247.5 && degrees > 202.5) gp.hat = GAMEPAD_HAT_DOWN_LEFT;
      if (degrees < 292.5 && degrees > 247.5) gp.hat = GAMEPAD_HAT_DOWN;
      if (degrees < 337.5 && degrees > 292.5) gp.hat = GAMEPAD_HAT_DOWN_RIGHT;
    }
  }

  // Z for the first button
  if (bZ) gp.buttons = (1U << 0);
  // C for second button
  if (bC) gp.buttons = (1U << 1);
  usb_hid.sendReport(0, &gp, sizeof(gp));
}

void is31_show_nunchuck() {
  const auto x = static_cast<int8_t>(7 * (jX - 127) / 255 + 3) - 2;
  const auto y = static_cast<int8_t>(7 * (jY - 127) / 255 + 3);
  x_pos = static_cast<int8_t>(6 + x);
  y_pos = static_cast<int8_t>(4 - y);
  ledmatrix.drawRect(0, 0, 9, 9, MODE_COLORS[mode]);
  if (x_pos_old != x_pos || y_pos_old != y_pos) {
    // clear the old pixel
    ledmatrix.drawPixel(x_pos_old, y_pos_old, BLACK);
    // save the new pixel
    x_pos_old = x_pos;
    y_pos_old = y_pos;
  }
  // draw the new pixel
  ledmatrix.drawPixel(x_pos, y_pos, MODE_COLORS[mode]);

  ledmatrix.drawRect(9, 0, 4, 4, MODE_COLORS[mode]);
  if (bC) {
    c_fill_color = C_COLOR;
  } else {
    c_fill_color = BLACK;
  }
  ledmatrix.fillRect(10, 1, 2, 2, c_fill_color);

  ledmatrix.drawFastHLine(9, 4, 4, MODE_COLORS[mode]);

  ledmatrix.drawRect(9, 5, 4, 4, MODE_COLORS[mode]);
  if (bZ) {
    z_fill_color = Z_COLOR;
  } else {
    z_fill_color = BLACK;
  }
  ledmatrix.fillRect(10, 6, 2, 2, z_fill_color);

  ledmatrix.show();
}

uint8_t x = 0;
uint8_t y = 0;
constexpr uint8_t WIDTH = 13;
constexpr uint8_t HEIGHT = 9;
bool down = true;

void update_game() {
  ledmatrix.drawPixel(x, y, BLACK);
  if (!(y % 2)) {
    x++;
    if (x >= WIDTH) {
      if (down) {
        y++;
      } else {
        y--;
      }
    }
  } else {
    if (x <= 0) {
      if (down) {
        y++;
      } else {
        y--;
      }
    }
    x--;
    if (y >= HEIGHT - 1) {
      down = false;
    }
    if (y <= 0) {
      down = true;
    }
  }
  delay(10);
  ledmatrix.drawPixel(x, y, WHITE);
}
