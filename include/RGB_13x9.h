#ifndef WIIHID_QT_PY_M0_PIO_RGB_13X9_H
#define WIIHID_QT_PY_M0_PIO_RGB_13X9_H

#include <Adafruit_IS31FL3741.h>
#include <Arduino.h>
#include <colors.h>
#include <modes.h>

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

struct position_t {
  short x, y;
};

// snake speed in pixels per second
constexpr short INITIAL_SNAKE_SPEED = 2;
constexpr short DEAD_ZONE = 64;

enum direction_t {
  NORTH, EAST, SOUTH, WEST
};


class RGB_13x9 {
public:
  static constexpr int WIDTH = 13;
  static constexpr int HEIGHT = 9;

  explicit RGB_13x9(
    int scaling = 0xFF,
    int current = 0x05,
    int rotation = 0,
    int refresh = 60,
    int address = 0x30
  );
  boolean begin();
  boolean is_attached() const;
  void clear();
  boolean update(ulong now, int mode, Accessory &nunchuck);
  static void decode_nunchuck(
    Accessory &nunchuck,
    int8_t &stick_x,
    int8_t &stick_y,
    boolean &button_z,
    boolean &button_c
  );
  void show_nunchuck(int mode, Accessory &nunchuck);
  void reset_game();
  void show_snake(ulong elapsed, Accessory nunchuck);

protected:
  Adafruit_IS31FL3741_QT matrix;
  int scaling;
  int current;
  int rotation;
  ulong delay;
  int address;
  const String NAME = "RGB Matrix (13x9, IS31FL3741)";
  boolean attached = false;
  ulong last_update = 0;
  int8_t stick_x = 0;
  int8_t stick_y = 0;
  boolean button_z = false;
  boolean button_c = false;
  ulong since_update = 0;
  ushort snake_speed = INITIAL_SNAKE_SPEED;
  position_t snake[13 * 9] = {{0, HEIGHT}};
  ushort length = 1;
  direction_t direction = EAST;
  boolean grow_apple = false;
  position_t apple_pos = {WIDTH / 2, HEIGHT / 2};
};

#endif //WIIHID_QT_PY_M0_PIO_RGB_13X9_H
