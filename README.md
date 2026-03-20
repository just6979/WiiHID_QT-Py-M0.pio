Using [Adafruit QT Py - SAMD21](https://www.adafruit.com/product/4600),
[WiiChuck library](https://github.com/madhephaestus/WiiChuck),
and various Adafruit Arduino libraries to turn a Wii Nunchuck
into a simple joystick/gamepad, and mouse emulator.
(selectable, not all at the same time, though that might be interesting...)

Using a [Adafruit IoT Button with NeoPixel BFF Add-On](https://www.adafruit.com/product/5666)
piggybacked on the QT PY to indicate and switch modes:

* Joystick/Gamepad Mode:
  * Nunchuck stick sends d-pad or left-stick signals
    * boots in joystick mode, one button press moves to d-pad mode
  * Z & C buttons send East (XBox B, Nintendo A) & South (XBox A, Nintendo B) button signals, respectively
* Mouse mode:
  * Two clicks to get to mouse mode
  * Nunchuck stick sends mouse move signals
    * Z & C buttons send Left & Right clicks, respectively
* Big Neopixel will indicate the mode:
  * [*L*ight] Blue for *L*eft-stick
  * *G*reen for D-*p*ad (*G*ame*p*ad)
  * *M*agenta/Purple for *M*ouse mode

* Next
  * Remember the mode
  * New modes:
    * Joystick on stick, mouse on accel
    * Accel mouse
    * everything all at once
  * Long press button in mouse mode to change sensitivity
  * configuration:
    * nunchuck stick deadzones to account for worn ones that are drifty
    * button mapping
    * perhaps expose storage as a USB drive, edit a file to configure 
  * More?
