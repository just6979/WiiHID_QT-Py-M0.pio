#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_TinyUSB.h>
#include <WiiChuck.h>
#include <Adafruit_IS31FL3741.h>
#include <Adafruit_Debounce.h>
#include <cmath>

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

const auto RED_555 = Adafruit_IS31FL3741_QT::color565(RED);
const auto ORANGE_555 = Adafruit_IS31FL3741_QT::color565(ORANGE);
const auto YELLOW_555 = Adafruit_IS31FL3741_QT::color565(YELLOW);
const auto GREEN_555 = Adafruit_IS31FL3741_QT::color565(GREEN);
const auto LIGHT_BLUE_555 = Adafruit_IS31FL3741_QT::color565(LIGHT_BLUE);
const auto BLUE_555 = Adafruit_IS31FL3741_QT::color565(BLUE);
const auto DARK_BLUE_555 = Adafruit_IS31FL3741_QT::color565(DARK_BLUE);
const auto INDIGO_555 = Adafruit_IS31FL3741_QT::color565(INDIGO);
const auto PURPLE_555 = Adafruit_IS31FL3741_QT::color565(PURPLE);
const auto WHITE_555 = Adafruit_IS31FL3741_QT::color565(WHITE);
const auto GRAY_555 = Adafruit_IS31FL3741_QT::color565(GRAY);
const auto BLACK_555 = Adafruit_IS31FL3741_QT::color565(BLACK);

constexpr int NEOPIXEL_STATUS_BRIGHTNESS = 1;
constexpr int NEOPIXEL_MODE_BRIGHTNESS = 5;

constexpr auto WII_UPDATE_DELAY = 2; // 500 Hz
constexpr auto HID_UPDATE_DELAY = 8; // 125 Hz
constexpr auto LED_UPDATE_DELAY = 16; // 60 Hz

constexpr int MODE_L_STICK = 0;
constexpr int MODE_D_PAD = 1;
constexpr int MODE_MOUSE = 2;
constexpr int MODE_GAME = 3;
constexpr int MODE_COUNT = 4;
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
int mode = MODE_GAME;

Adafruit_NeoPixel neopixel_status(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel neopixel_mode(1, PIN_A3, NEO_GRB + NEO_KHZ800);
Adafruit_Debounce button_mode(PIN_A2, LOW);

TwoWire *i2c = &Wire;

Accessory nunchuck;
bool nunchuck_found = false;
int NUNCHUCK_ADDRESS = 0x52;
int8_t stick_x = 0;
int8_t stick_y = 0;
bool button_z = false;
bool button_c = false;

constexpr uint8_t desc_hid_report[] = {
  TUD_HID_REPORT_DESC_GAMEPAD()
};
Adafruit_USBD_HID usb_hid;
hid_gamepad_report_t gamepad_report;

Adafruit_IS31FL3741_QT is31;
bool is31_found = false;
constexpr int IS31_ADDRESS = 0x30;
constexpr int IS31_LED_SCALING = 0xFF;
constexpr int IS31_GLOBAL_CURRENT = 0x01;
constexpr int IS31_WIDTH = 13;
constexpr int IS31_HEIGHT = 9;

boolean reset_game = true;

void set_mode(int new_mode);
void next_mode();
void check_nunchuck();
bool update_wii_acc();
void update_usb_hid();
void is31_show_nunchuck();
void update_game(ulong elapsed);


void setup() {
  Serial.begin(115200);

#if DEBUG
  const auto start = millis();
  while (!Serial) {
    if (millis() - start > 2000) break;
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

  check_nunchuck();

  if (is31.begin(IS31_ADDRESS, i2c)) {
    Serial.printf("IS41 found at 0x%X\n", IS31_ADDRESS);
    is31_found = true;
    is31.setLEDscaling(IS31_LED_SCALING);
    is31.setGlobalCurrent(IS31_GLOBAL_CURRENT);
    is31.enable(true);
    is31.fill(BLACK);
    is31.show();
  } else {
    Serial.printf("IS41 not found at 0x%X\n", IS31_ADDRESS);
  }

  Serial.println("Set up Big NeoPixel (button side) to show mode");
  neopixel_mode.begin();
  neopixel_mode.setBrightness(NEOPIXEL_MODE_BRIGHTNESS);

  Serial.println("Set up Big Button to change modes");
  button_mode.begin();
  set_mode(mode);

  srand(millis());
  Serial.println("Done setup");
}


void loop() {
  static ulong now;
  static ulong last_wii_update = 0;
  static ulong last_hid_update = 0;
  static ulong last_led_update = 0;

  now = millis();

  button_mode.update();
  if (button_mode.justReleased()) {
    next_mode();
  }

  if (mode != MODE_GAME) {
    if (!TinyUSBDevice.mounted()) {
      Serial.println("USB HID not mounted, trying again");
      TinyUSBDevice.attach();
      neopixel_status.setPixelColor(0, RED);
      return;
    }
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
    const ulong elapsed = now - last_led_update;
    if (elapsed >= LED_UPDATE_DELAY) {
      if (mode != MODE_GAME) {
        is31_show_nunchuck();
      } else {
        update_game(elapsed);
      }
      last_led_update = now;
    }
  }
}

void set_mode(const int new_mode) {
  mode = new_mode;

  neopixel_mode.setPixelColor(0, MODE_COLORS[mode]);
  neopixel_mode.show();

  is31.fill(BLACK);

  if (mode == MODE_GAME) {
    reset_game = true;
  }

  Serial.print("Changed mode to ");
  Serial.println(MODE_NAMES[mode]);
}

void next_mode() {
  int new_mode = mode + 1;
  Serial.println(new_mode);
  if (new_mode == MODE_MOUSE) {
    // TODO: remove this skip when mouse mode is implemented
    Serial.println("Mouse mode not yet implemented.");
    new_mode++;
  }
  Serial.println(new_mode);
  if (new_mode == MODE_GAME && !is31_found) {
    Serial.print("Can't play game without LED matrix plugged in.");
    new_mode++;
  }
  Serial.println(new_mode);
  if (new_mode >= MODE_COUNT) {
    new_mode = 0;
  }
  Serial.println(new_mode);
  set_mode(new_mode);
}

void check_nunchuck() {
  Serial.println("Checking for Wii accesorries");
  nunchuck.begin();
  if (nunchuck.type == NUNCHUCK) {
    Serial.println("Found Nunchuck");
    nunchuck_found = true;
    neopixel_status.fill(GREEN);
    neopixel_status.show();
  } else {
    Serial.printf("Missing or unknown accessory (type:%d)", nunchuck.type);
    Serial.println();
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
    delay(2000);
    nunchuck.reset();
    check_nunchuck();
  }
  if (!nunchuck.isConnected()) {
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
    stick_x = static_cast<int8_t>(nunchuck.getJoyX() - 128);
    stick_y = static_cast<int8_t>(nunchuck.getJoyY() - 128);
    if (stick_x < -127) { stick_x = -127; }
    if (stick_y < -127) { stick_y = -127; }
    stick_y *= -1;
    button_z = nunchuck.getButtonZ();
    button_c = nunchuck.getButtonC();
#if DEBUG && SHOW_NUNCHUCK
    const int accel_x = nunchuck.getAccelX();
    const int accel_y = nunchuck.getAccelY();
    const int accel_y = nunchuck.getAccelZ();
    Serial.printf(
      "J[%4d,%4d];A[%3d,%3d,%3d];B[%s%s]",
      stick_x,
      stick_y,
      accel_x,
      accel_y,
      accel_y,
      button_z ? "Z" : " ",
      button_c ? "C" : " "
    );
    Serial.println();
#endif
  }
  return true;
}

void is31_show_nunchuck() {
  static short show_x = 4;
  static short show_y = 4;
  const auto MODE_COLOR_555 = Adafruit_IS31FL3741_QT::color565(
    MODE_COLORS[mode]
  );

  // show stick position with a dot in a 9x9 box on the left
  is31.drawRect(0, 0, 9, 9, MODE_COLOR_555);
  is31.drawPixel(show_x, show_y, BLACK_555);
  if (mode == MODE_L_STICK) {
    show_x = static_cast<short>((7 * (stick_x - 127) / 255) + 7);
    show_y = static_cast<short>((7 * (stick_y - 127) / 255) + 7);
  } else if (mode == MODE_D_PAD) {
    show_x = show_y = 4;
    if (stick_x >= 64) show_x += 3;
    if (stick_x <= -64) show_x -= 3;
    if (stick_y >= 64) show_y += 3;
    if (stick_y <= -64) show_y -= 3;
  }
  is31.drawPixel(show_x, show_y, MODE_COLOR_555);
  // show button presses in two 4x4 boxes on the right
  is31.drawRect(9, 0, 4, 4, MODE_COLOR_555);
  is31.fillRect(10, 1, 2, 2, button_c ? ORANGE_555 : BLACK_555);
  is31.drawRect(9, 5, 4, 4, MODE_COLOR_555);
  is31.fillRect(10, 6, 2, 2, button_z ? PURPLE_555 : BLACK_555);
  // draw a line between the button boxes
  is31.drawFastHLine(9, 4, 4, MODE_COLOR_555);

  is31.show();
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
    gamepad_report.x = stick_x;
    gamepad_report.y = stick_y;
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

void update_game(const ulong elapsed) {
  // snake speed in pixels per second
  constexpr short INITIAL_SNAKE_SPEED = 2;
  constexpr short DEAD_ZONE = 64;
  enum direction_t {
    NORTH, EAST, SOUTH, WEST
  };

  struct position_t {
    short x, y;
  };

  static ulong since_update = 0;
  static short snake_speed = INITIAL_SNAKE_SPEED;
  static position_t head = {0, IS31_HEIGHT / 2};
  static position_t body[13 * 9] = {head};
  static ushort length = 1;
  static direction_t direction = EAST;
  static boolean grow_apple = true;
  static position_t apple_pos;

  if (reset_game) {
    since_update = 0;
    snake_speed = INITIAL_SNAKE_SPEED;
    head = {0, IS31_HEIGHT / 2};
    body[0] = {head};
    length = 1;
    direction = EAST;
    reset_game = false;
  }

  // stick position and direction are updated at full rate
  if ((abs(stick_x)) >= (abs(stick_y))) {
    // horizontal deflection is bigger or equal
    // equal goes to horizontal because screen aspect is wider than tall
    if (stick_x > DEAD_ZONE && direction != WEST) direction = EAST;
    if (stick_x < -DEAD_ZONE && direction != EAST) direction = WEST;
  } else {
    // vertical deflection is bigger
    if (stick_y > DEAD_ZONE && direction != SOUTH) direction = NORTH;
    if (stick_y < -DEAD_ZONE && direction != NORTH) direction = SOUTH;
  }

  // game state is updated based on snake speed
  since_update += elapsed;
  if (since_update < 1000 / snake_speed) {
    // too early, don't update
    return;
  }
  since_update = 0;
  // it's time to update the game state

  is31.drawPixel(head.x, head.y, BLACK_555);

  switch (direction) {
    case NORTH:
      head.y += 1;
      break;
    case EAST:
      head.x += 1;
      break;
    case SOUTH:
      head.y -= 1;
      break;
    case WEST:
      head.x -= 1;
      break;
  }

  if (head.x >= IS31_WIDTH) {
    head.x = 0;
  };
  if (head.x < 0) {
    head.x = IS31_WIDTH - 1;
  }

  if (head.y >= IS31_HEIGHT) {
    head.y = 0;
  }
  if (head.y < 0) {
    head.y = IS31_HEIGHT - 1;
  }

  is31.drawPixel(head.x, head.y, GREEN_555);

  if (head.x == apple_pos.x && head.y == apple_pos.y) {
    snake_speed++;
    grow_apple = true;
  }

  if (grow_apple) {
    apple_pos.x = rand() % IS31_WIDTH;
    apple_pos.y = rand() % IS31_HEIGHT;
    grow_apple = false;
  }

  is31.drawPixel(apple_pos.x, apple_pos.y, RED_555);
}
