# Air quality monitor (Arduino Nano 33 IOT &amp; HMPA115)

Reads values from HPMA115 particulate matter sensor, displays air quality index to LED ring and sends data to ThingSpeak and Telegram.

## BOM
 * [HMPA115 sensor](https://sensing.honeywell.com/HPMA115C0-003-particulate-matter-sensors)
 * [SFSD Cable](https://ch.rs-online.com/web/p/kabel-platine-kabelkomponenten/1800034/)
 * [Arduino Nano 33 IOT](https://store.arduino.cc/arduino-nano-33-iot)
 * [Adafruit Neopixel Ring](https://www.adafruit.com/product/1463)
 * Mini Breadboard
 * Wiring
 
 ## Connections
 
 First, enable VUSB 5V output by soldering the two pads at the bottom of the Nano board. Establish connections according to below table. Use a breadboard and wiring as needed. Refer to the [HPMA datasheet](https://sensing.honeywell.com/honeywell-sensing-particulate-hpm-series-datasheet-32322550.pdf) for pin definitions and use the SFSD cable for easy connections.
 
 | Nano | HPMA | LED |
 |------|------|-----|
 | TX1  |  9   |     |
 | RX0  |  7   |     |
 | GND  |  4   | GND |
 | VUSB |  2   | VCC |
 |  D2  |      |  IN |
 
## Required libraries
 * [UniversalTelegramBot](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot)
 * [Adafruit Neopixel](https://github.com/adafruit/Adafruit_NeoPixel)
 * [HPMA115_Compact](https://github.com/jedp/PMSensor-HPMA115)
 * [WiFiNINA](https://www.arduino.cc/en/Reference/WiFiNINA)
 * [ThingSpeak](https://www.arduino.cc/reference/en/libraries/thingspeak/)

## Pasted together from different sources:
  * https://github.com/ostaquet/Arduino-Nano-33-IoT-Ultimate-Guide
  * https://github.com/jedp/PMSensor-HPMA115
  * https://github.com/mathworks/thingspeak-arduino
  * https://github.com/adafruit/Adafruit_NeoPixel
  * https://create.arduino.cc/projecthub/AppsByDavideV/iot-telegram-bot-with-arduino-mkr-wifi-1010-b835a4
  * https://www.mouser.ch/datasheet/2/187/honeywell-sensing-particulate-hpm-series-datasheet-1609244.pdf
  * https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
  * https://forum.arduino.cc/index.php?topic=447035.0
