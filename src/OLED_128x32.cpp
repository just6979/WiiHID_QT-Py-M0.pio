#include <OLED_128x32.h>

OLED_128x32::OLED_128x32(
  TwoWire *i2c,
  const int rotation,
  const int speed,
  const int address
) : screen(WIDTH, HEIGHT, i2c),
    rotation(rotation),
    delay(1000 / speed),
    address(address) {
}

boolean OLED_128x32::begin() {
  if (screen.begin(SSD1306_SWITCHCAPVCC, address, true, false)) {
    Serial.printf(
      "%s: R:%d, D:%d, A:0x%x\n",
      NAME.c_str(), rotation, delay, address
    );
    attached = true;
    screen.setRotation(rotation);
    screen.setTextWrap(false);
    screen.setTextSize(4);
    screen.setTextColor(WHITE);
  } else {
    Serial.printf("%s NOT found at 0x%X\n", NAME.c_str(), address);
    attached = false;
  }

  scroll_x = 0;
  screen.clearDisplay();
  screen.display();
  return attached;
}

boolean OLED_128x32::is_attached() const {
  return attached;
}

int OLED_128x32::set_text(const String &new_text, const boolean reset) {
  short temp_x, temp_y;
  ushort temp_h;

  text = new_text;
  screen.getTextBounds(
    text, 0, 0, &temp_x, &temp_y, &scroll_width, &temp_h
  );

  if (reset) {
    scroll_x = WIDTH;
  }

  Serial.printf(
    "%s: text set to '%s', width: %d\n",
    NAME.c_str(), text.c_str(), scroll_width
  );

  return this->scroll_width;
}

boolean OLED_128x32::update(const ulong now) {
  if (!attached || now - last_update < delay) {
    return false;
  }
  last_update = now;

  screen.clearDisplay();
  screen.setCursor(scroll_x, 1);
  screen.print(text);
  screen.display();
  if (scroll_x-- < -scroll_width + 2) {
    scroll_x = screen.width();
  }

  return true;
}
