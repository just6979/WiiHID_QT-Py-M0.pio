#include <Adafruit_Debounce.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_TinyUSB.h>
#include <Arduino.h>
#include <cmath>
#include <WiiChuck.h>

// #define SHOW_NUNCHUCK

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

constexpr int MODE_L_STICK = 0;
constexpr int MODE_D_PAD = 1;
constexpr int MODE_MOUSE = 2;
constexpr int MODE_SNAKE = 3;
constexpr int MODE_COUNT = 4;
const String MODE_NAMES[MODE_COUNT] = {
  "L-STICK",
  "D-PAD",
  "MOUSE",
  "SNAKE"
};
const uint32_t MODE_COLORS[MODE_COUNT] = {
  // Left-stick
  LIGHT_BLUE,
  // D-pad
  DARK_BLUE,
  // "Magenta" for Mouse
  PURPLE,
  // Snake Game
  GREEN
};

// helper to check memory usage
extern "C" char *sbrk(int i);

int mem_free() {
  char stack_dummy = 0;
  return &stack_dummy - sbrk(0);
}

constexpr int NEOPIXEL_STATUS_BRIGHTNESS = 1;
constexpr int NEOPIXEL_MODE_BRIGHTNESS = 5;

constexpr auto WII_UPDATE_DELAY = 2; // 500 Hz
constexpr auto HID_UPDATE_DELAY = 8; // 125 Hz

int mode = MODE_L_STICK;

Adafruit_NeoPixel neopixel_status(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel neopixel_mode(1, PIN_A3, NEO_GRB + NEO_KHZ800);
Adafruit_Debounce button_mode(PIN_A2, LOW);

TwoWire *i2c = &Wire;

Accessory nunchuck;
bool nunchuck_found = false;
int NUNCHUCK_ADDRESS = 0x52;
int stick_x = 0;
int stick_y = 0;
int accel_x = 0;
int accel_y = 0;
int accel_z = 0;
bool button_z = false;
bool button_c = false;

constexpr uint8_t desc_hid_report[] = {
  TUD_HID_REPORT_DESC_GAMEPAD()
};
Adafruit_USBD_HID usb_hid;
hid_gamepad_report_t gamepad_report;

boolean nunchuck_present = false;
boolean should_check_nunchuck = false;
ulong last_nunchuck_check = 0;
ulong nunchuck_check_delay = 2000;

boolean reset_game = true;

void set_mode(const int new_mode) {
  mode = new_mode;

  neopixel_mode.setPixelColor(0, MODE_COLORS[mode]);
  neopixel_mode.show();

  Serial.printf("Changed mode to %s\n", MODE_NAMES[mode].c_str());
  Serial.printf("Free RAM: %d bytes\n", mem_free());
}

void next_mode() {
  int new_mode = mode + 1;
  if (new_mode == MODE_MOUSE) {
    // TODO: remove this skip when mouse mode is implemented
    Serial.println("Mouse mode not yet implemented.");
    new_mode++;
  }
  if (new_mode >= MODE_COUNT) {
    new_mode = 0;
  }
  set_mode(new_mode);
}

void check_nunchuck() {
  Serial.println("Checking for Wii accessories");
  nunchuck.begin();
  Serial.printf("type = %d\n", nunchuck.type);
  if (nunchuck.isConnected() && nunchuck.type == NUNCHUCK) {
    Serial.println("Found Nunchuck");
    nunchuck_found = true;
    neopixel_status.fill(GREEN);
    neopixel_status.show();
  } else {
    Serial.printf("Missing or unknown accessory (type: %d)\n", nunchuck.type);
    nunchuck_found = false;
    neopixel_status.fill(YELLOW);
    neopixel_status.show();
  }
}

bool update_wii_acc() {
  if (!nunchuck.isConnected()) {
    Serial.println(
      "No known Wii accessory connected, trying again in 2 seconds."
    );
    neopixel_status.setPixelColor(0, YELLOW);
    neopixel_status.show();
    should_check_nunchuck = true;
    nunchuck_found = false;
    last_nunchuck_check = millis() - nunchuck_check_delay;
    return false;
  }

  if (!nunchuck.readData()) {
    Serial.println("Could not read data from Wii Accessory, trying again");
    neopixel_status.setPixelColor(0, YELLOW);
    neopixel_status.show();
    return false;
  }

  if (nunchuck.type == NUNCHUCK) {
    neopixel_status.setPixelColor(0, GREEN);
    neopixel_status.show();
    stick_x = nunchuck.getJoyX() - 128;
    stick_y = nunchuck.getJoyY() - 128;
    if (stick_x < -127) { stick_x = -127; }
    if (stick_y < -127) { stick_y = -127; }
    stick_y *= -1;
    accel_x = nunchuck.getAccelX();
    accel_y = nunchuck.getAccelY();
    accel_z = nunchuck.getAccelZ();
    button_z = nunchuck.getButtonZ();
    button_c = nunchuck.getButtonC();
#if DEBUG && SHOW_NUNCHUCK
    Serial.printf(
      "J[%4d,%4d];A[%3d,%3d,%3d];B[%s%s]\n",
      stick_x,
      stick_y,
      accel_x,
      accel_y,
      accel_y,
      button_z ? "Z" : " ",
      button_c ? "C" : " "
    );
#endif
  }
  return true;
}


void update_usb_hid() {
  static double theta;
  static double degrees;
  static int val;

  // reset
  gamepad_report.x = 0;
  gamepad_report.y = 0;
  gamepad_report.hat = 0;
  gamepad_report.buttons = 0;

  if (mode == MODE_L_STICK) {
    gamepad_report.x = static_cast<int8_t>(stick_x);
    gamepad_report.y = static_cast<int8_t>(stick_y);
  }

  if (mode == MODE_D_PAD) {
    theta = atan2(stick_y, stick_x);
    degrees = theta * (180.0 / M_PI);
    degrees += degrees < 0 ? 360 : 0;
    val = static_cast<int>(sqrt(pow(stick_x, 2) + pow(stick_y, 2)));
    if (val > 64) {
      if (degrees < 22.5 || degrees > 337.5)
        gamepad_report.hat = GAMEPAD_HAT_RIGHT;
      if (degrees < 67.5 && degrees > 22.5)
        gamepad_report.hat = GAMEPAD_HAT_UP_RIGHT;
      if (degrees < 112.5 && degrees > 67.5)
        gamepad_report.hat = GAMEPAD_HAT_UP;
      if (degrees < 157.5 && degrees > 112.5)
        gamepad_report.hat = GAMEPAD_HAT_UP_LEFT;
      if (degrees < 202.5 && degrees > 157.5)
        gamepad_report.hat = GAMEPAD_HAT_LEFT;
      if (degrees < 247.5 && degrees > 202.5)
        gamepad_report.hat = GAMEPAD_HAT_DOWN_LEFT;
      if (degrees < 292.5 && degrees > 247.5)
        gamepad_report.hat = GAMEPAD_HAT_DOWN;
      if (degrees < 337.5 && degrees > 292.5)
        gamepad_report.hat = GAMEPAD_HAT_DOWN_RIGHT;
    }
  }

  // Z for the first button
  if (button_z) gamepad_report.buttons = (1U << 0);
  // C for second button
  if (button_c) gamepad_report.buttons = (1U << 1);

  // TODO: don't repeat sending identical reports
  usb_hid.sendReport(0, &gamepad_report, sizeof(gamepad_report));
}


void setup() {
  // setup TinyUSB HID first to avoid the serial monitor disconnecting
  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.begin();
  // reattach to ensure all drivers start
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }

  Serial.begin(115200);
#if DEBUG
  // wait for serial for monitoring setup
  ulong serial_time = millis();
  while (!Serial) {
    // but don't wait forever in case a debug build was left on the device
    if (serial_time > 4000) break;
    serial_time = millis();
  }
  Serial.println("Starting");
  Serial.printf("Free RAM at start: %d bytes\n", mem_free());
#endif

  Serial.println("Set up Little NeoPixel (board side) to show status");
  neopixel_status.begin();
  neopixel_status.setBrightness(NEOPIXEL_STATUS_BRIGHTNESS);
  neopixel_status.fill(YELLOW);
  neopixel_status.show();

  Serial.println("Set up Big NeoPixel (button side) to show mode");
  neopixel_mode.begin();
  neopixel_mode.setBrightness(NEOPIXEL_MODE_BRIGHTNESS);

  Serial.println("Set up Big Button to change modes");
  button_mode.begin();

  check_nunchuck();

  srand(micros());

#ifdef DEBUG
  Serial.printf("Setup done in %d ms\n", micros() - serial_time);
  Serial.printf("Free RAM: %d bytes\n", mem_free());
#endif
}

void loop() {
  static ulong now;
  static ulong last_wii_update = 0;
  static ulong last_hid_update = 0;

  now = millis();

  button_mode.update();
  if (button_mode.justReleased()) {
    next_mode();
  }

  if (mode != MODE_SNAKE) {
    if (!TinyUSBDevice.mounted()) {
      Serial.println("USB HID not mounted, trying again");
      TinyUSBDevice.attach();
      neopixel_status.setPixelColor(0, RED);
      return;
    }
  }

  if (should_check_nunchuck && now - last_nunchuck_check >
      nunchuck_check_delay) {
    Serial.println("Checking for nunchucks");
    check_nunchuck();
    if (nunchuck.isConnected()) {
      Serial.println("Found a nunchuck");
      should_check_nunchuck = false;
      nunchuck_found = true;
    } else {
      Serial.println("No nunchuck found");
      should_check_nunchuck = true;
      nunchuck_found = false;
      // nunchuck.reset();
    }
  }

  if (nunchuck_found && now - last_wii_update >= WII_UPDATE_DELAY) {
    update_wii_acc();
    last_wii_update = now;
  }

  if (now - last_hid_update >= HID_UPDATE_DELAY) {
    if (usb_hid.ready()) {
      update_usb_hid();
    }
    last_hid_update = now;
  }
}
