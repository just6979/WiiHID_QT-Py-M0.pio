Using [Adafruit QT Py - SAMD21](https://www.adafruit.com/product/4600),
[WiiChuck library](https://github.com/madhephaestus/WiiChuck),
and various Adafruit Arduino libraries, to turn a Wii Nunchuck
into a simple joystick/gamepad and* mouse emulator.

Using a [Adafruit IoT Button with NeoPixel BFF Add-On](https://www.adafruit.com/product/5666)
piggybacked on the QT PY to indicate and switch modes:

Modes:
  * Nunchuck
    * L-Stick
      * Nunchuck stick Left-stick signals
      * Z sends Button 1 (South, XBox A, Nintendo B, PS Circle)
      * C sends Button 2 (East, XBox B, Nintendo A, PS X)
    * Gamepad
      * Nunchuck stick sends d-pad/hat signals
      * Buttons same as L-Stick Mode 
    * Mouse
      * Nunchuck stick sends mouse move signals
        * Z & C buttons send Left & Right clicks, respectively
        * ? Both together send middle click ?
    * Game
      * If the IS31FL3741 I2C LED Matrix is attached, use it to play a simple Snake Game
      * Change direction with stick
    * ? R-Stick ?
      * Same as L-Stick but send Right Stick
      * ? Maybe ?
  * Classic
    * Pass
      * Just acts like a gamepad
      * 2 axes, 2 triggers (3rd axis?), hat, 9 buttons (7 face, 2 shoulder)
        * No clickable sticks, so no L3/R3.
        * ? Treat full trigger pulls as 2 more buttons like on the GameCube controller ?
    * Flip
      * Same as Pass-through but L-Stick and D-Pad are swapped
    * Mixed
      * Left stick sends left stick
      * D-pad sends D-pad
      * Right stick sends mouse
      * Buttons send normal gamepad buttons, plus:
        * Full trigger pulls send left & right mouse clicks, either Z button sends middle click.
    * ? All Mouse ?
      * Both sticks and D-pad send mouse movement
      * Full trigger pulls send left & right mouse clicks, either Z button sends middle click
      * Y or Minus: LMB, A or Plus: RMB, X, B, or Home: MMB
      
* Big Neopixel will indicate the mode:
  * Nunchuck
    * [*L*ight] Blue for *L*-stick
    * *D*ark Blue for *D*-Pad
    * *M*agenta for *M*ouse
    * *G*reen for *G*ame
  * Classic
    * *P*ink for *P*ass
    * *F*uscia for *F*lip
    * *M*agenta for *M*ixed
    * If All Mouse exists:
      * White for Mixed (all colors for all modes)
      * *M*agenta is for All *M*ouse

* Next
  * Remember the mode
  * New modes:
    * Accel mouse
    * Joystick on stick, mouse on accel
    * All modes at once
  * Long press button in mouse mode to change sensitivity
  * configuration:
    * filesystem over USB?
    * nunchuck stick deadzones to account for worn ones that are drifty
    * button mapping
