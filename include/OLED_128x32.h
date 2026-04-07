#ifndef WIIHID_QT_PY_M0_PIO_OLED_128X32_H
#define WIIHID_QT_PY_M0_PIO_OLED_128X32_H

#include <Adafruit_SSD1306.h>
#include <Arduino.h>

class OLED_128x32 {
public:
  static constexpr auto WIDTH = 128;
  static constexpr auto HEIGHT = 32;

  explicit OLED_128x32(
    TwoWire *i2c,
    int rotation = 2,
    int speed = 60,
    int address = 0x3C
  );
  boolean begin();
  boolean is_attached() const;
  int set_text(const String &new_text, boolean reset = true);
  boolean update(ulong now);

protected:
  Adafruit_SSD1306 screen;
  int rotation;
  ulong delay;
  int address;
  const String NAME = "OLED Screen (128x32, SSD1306)";
  boolean attached = false;
  ulong last_update = 0;
  short scroll_x = 0;
  ushort scroll_width = 0;
  String text = "";
};

#endif //WIIHID_QT_PY_M0_PIO_OLED_128X32_H
