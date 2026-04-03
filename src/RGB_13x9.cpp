#include <RGB_13x9.h>

RGB_13x9::RGB_13x9(
  const int scaling,
  const int current,
  const int rotation,
  const int refresh,
  const int address
) : scaling(scaling),
    current(current),
    rotation(rotation),
    delay(1000 / refresh),
    address(address) {
}

boolean RGB_13x9::begin() {
  if (matrix.begin(address)) {
    Serial.printf(
      "%s - S:0x%x, C:0x%x R:%d, D:%d, A:0x%x\n",
      NAME.c_str(), scaling, current, rotation, delay, address
    );
    attached = true;
    matrix.setRotation(rotation);
    matrix.setLEDscaling(scaling);
    matrix.setGlobalCurrent(current);
    matrix.enable(true);
    clear();
  } else {
    Serial.printf("%s NOT found at 0x%X\n", NAME.c_str(), address);
    attached = false;
  }

  return attached;
}

boolean RGB_13x9::is_attached() const {
  return attached;
}

void RGB_13x9::clear() {
  matrix.fill(BLACK_555);
  // matrix.show();
}

boolean RGB_13x9::update(
  const ulong now,
  const int mode,
  Accessory &nunchuck
) {
  const ulong elapsed = now - last_update;

  if (!attached || elapsed < delay) {
    return false;
  }
  last_update = now;

  if (mode != MODE_SNAKE) {
    show_nunchuck(mode, nunchuck);
  } else {
    show_snake(elapsed, nunchuck);
  }

  return true;
}

void RGB_13x9::decode_nunchuck(
  Accessory &nunchuck,
  int8_t &stick_x,
  int8_t &stick_y,
  boolean &button_z,
  boolean &button_c
) {
  stick_x = static_cast<int8_t>(nunchuck.getJoyX() - 128);
  stick_y = static_cast<int8_t>(nunchuck.getJoyY() - 128);
  if (stick_x < -127) { stick_x = -127; }
  if (stick_y < -127) { stick_y = -127; }
  stick_y *= -1;
  button_z = nunchuck.getButtonZ();
  button_c = nunchuck.getButtonC();
}

void RGB_13x9::show_nunchuck(const int mode, Accessory &nunchuck) {
  static short show_x = 4;
  static short show_y = 4;
  const auto MODE_COLOR_555 = Adafruit_IS31FL3741_QT::color565(
    MODE_COLORS[mode]
  );

  decode_nunchuck(nunchuck, stick_x, stick_y, button_z, button_c);

  // show stick position with a dot in a 9x9 box on the left
  matrix.drawRect(0, 0, 9, 9, MODE_COLOR_555);
  matrix.drawPixel(show_x, show_y, BLACK_555);
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
  matrix.drawPixel(show_x, show_y, MODE_COLOR_555);
  // show button presses in two 4x4 boxes on the right
  matrix.drawRect(9, 0, 4, 4, MODE_COLOR_555);
  matrix.fillRect(10, 1, 2, 2, button_c ? ORANGE_555 : BLACK_555);
  matrix.drawRect(9, 5, 4, 4, MODE_COLOR_555);
  matrix.fillRect(10, 6, 2, 2, button_z ? PURPLE_555 : BLACK_555);
  // draw a line between the button boxes
  matrix.drawFastHLine(9, 4, 4, MODE_COLOR_555);

  matrix.show();
}

void RGB_13x9::reset_game() {
  since_update = 0;
  snake_speed = INITIAL_SNAKE_SPEED;
  snake[0] = {0, HEIGHT};
  length = 1;
  direction = EAST;
  grow_apple = true;
  // clear();
}

void RGB_13x9::show_snake(
  const ulong elapsed,
  Accessory nunchuck
) {
  decode_nunchuck(nunchuck, stick_x, stick_y, button_z, button_c);

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

  for (int i = 0; i < length; i++) {
    matrix.drawPixel(snake[i].x, snake[i].y, BLACK_555);
  }

  switch (direction) {
    case NORTH:
      snake[0].y += 1;
      break;
    case EAST:
      snake[0].x += 1;
      break;
    case SOUTH:
      snake[0].y -= 1;
      break;
    case WEST:
      snake[0].x -= 1;
      break;
  }

  if (snake[0].x >= WIDTH) {
    snake[0].x = 0;
  };
  if (snake[0].x < 0) {
    snake[0].x = WIDTH - 1;
  }

  if (snake[0].y >= HEIGHT) {
    snake[0].y = 0;
  }
  if (snake[0].y < 0) {
    snake[0].y = HEIGHT - 1;
  }

  for (int i = 0; i < length; i++) {
    matrix.drawPixel(snake[i].x, snake[i].y, GREEN_555);
  }

  if (snake[0].x == apple_pos.x && snake[0].y == apple_pos.y) {
    snake_speed++;
    grow_apple = true;
  }

  if (grow_apple) {
    apple_pos.x = rand() % WIDTH;
    apple_pos.y = rand() % HEIGHT;
    grow_apple = false;
  }

  matrix.drawPixel(apple_pos.x, apple_pos.y, RED_555);

  // matrix.show();
}
