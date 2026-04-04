#include <LED_8x8.h>

LED_8x8::LED_8x8(
  const int brightness,
  const int rotation,
  const int speed,
  const int address
) : brightness(brightness),
    rotation(rotation),
    delay(1000 / speed),
    address(address) {
}

boolean LED_8x8::begin() {
  if (matrix.begin(address)) {
    Serial.printf(
      "%s: B:%d, R:%d, D:%d, A:0x%x\n",
      NAME.c_str(), brightness, rotation, delay, address
    );
    attached = true;
    matrix.setRotation(rotation);
    matrix.setBrightness(brightness);
    matrix.setTextWrap(false);
    matrix.setTextColor(LED_ON);
  } else {
    Serial.printf("%s NOT found at 0x%X\n", NAME.c_str(), address);
    attached = false;
  }

  scroll_x = 0;
  matrix.clear();
  return attached;
}

boolean LED_8x8::is_attached() const {
  return attached;
}

int LED_8x8::set_text(const String &new_text, const boolean reset) {
  short temp_x, temp_y;
  ushort temp_h;

  text = new_text;
  matrix.getTextBounds(
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

boolean LED_8x8::update(const ulong now) {
  if (attached && now - last_update < delay) {
    return false;
  }
  last_update = now;

  matrix.clear();
  matrix.setCursor(scroll_x, 1);
  matrix.print(text);
  matrix.writeDisplay();
  if (scroll_x-- < -scroll_width) {
    scroll_x = matrix.width();
  }

  return true;
}
