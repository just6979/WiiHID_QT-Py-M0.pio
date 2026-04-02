#ifndef WIIHID_QT_PY_M0_PIO_LED_8X8_H
#define WIIHID_QT_PY_M0_PIO_LED_8X8_H

#include <Adafruit_LEDBackpack.h>
#include <Arduino.h>

class LED_8x8 {
public:
  static constexpr int WIDTH = 8;
  static constexpr int HEIGHT = 8;

  explicit LED_8x8(
    int brightness = 0,
    int rotation = 3,
    int speed = 15,
    int address = 0x72
  );
  boolean begin();
  boolean is_attached() const;
  int set_text(const String &new_text, boolean reset = true);
  boolean update(ulong now);

protected:
  Adafruit_8x8matrix matrix;
  int brightness;
  int rotation;
  ulong delay;
  int address;
  const String NAME = "LED Matrix (8x8, HT16K33)";
  boolean attached = false;
  ulong last_update = 0;
  short scroll_x = 0;
  ushort scroll_width = 0;
  String text = "";
};

#endif //WIIHID_QT_PY_M0_PIO_LED_8X8_H
