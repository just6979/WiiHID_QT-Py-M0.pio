#ifndef WIIHID_QT_PY_M0_PIO_MODES_H
#define WIIHID_QT_PY_M0_PIO_MODES_H

#include <Accessory.h>
#include <colors.h>

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

#endif //WIIHID_QT_PY_M0_PIO_MODES_H
