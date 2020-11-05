/*
  surgery_airq

  Measures air quality using HPMA115 sensor, posts readings to 
  Thingspeak and uses an LED ring to notify about Air Quality index.

  Created 04 November 2020
  By Kim Ferrari

  http://github.com/kferrari

*/

#include <Arduino.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include "wiring_private.h"

#include <HPMA115_Compact.h>
#include <Adafruit_NeoPixel.h>

#include <ThingSpeak.h>

// WiFi settings
char ssid[] = SECRET_SSID; //  your network SSID (name)
char pass[] = SECRET_PASS; // your network password
int status = WL_IDLE_STATUS;

// ThingSpeak Settings
char server[] = "api.thingspeak.com";
String writeAPIKey = SECRET_API_KEY;
unsigned long lastConnectionTime = 0; // track the last connection time
const unsigned long postingInterval = 60L * 1000L; // post data every 60 seconds

// HPMA settings and variables
#define HPMA_RX 5
#define HPMA_TX 6
uint16_t pm1, pm25, pm4, pm10, aqi, lastAqi;

// Neopixel settigns
#define LEDPIN 2
#define NUMPIXELS 12

// Initialize the Wifi client library
WiFiClient client;

// Initialize NeoPixel ring
Adafruit_NeoPixel ring = Adafruit_NeoPixel(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

// Create a hardware serial on pins 5 (RX) and 6 (TX)
Uart hpmSerial (&sercom0, HPMA_RX, HPMA_TX, SERCOM_RX_PAD_1, UART_TX_PAD_0);

// Create an object to communicate with the HPM Compact sensor.
HPMA115_Compact hpm = HPMA115_Compact();

void setup() {
  // Reassign pins 5 and 6 to SERCOM alt
  pinPeripheral(5, PIO_SERCOM_ALT);
  pinPeripheral(6, PIO_SERCOM_ALT);

  // Console serial.
  Serial.begin(HPMA115_BAUD);

  // Serial for ineracting with HPM device.
  hpmSerial.begin(HPMA115_BAUD);

  // Configure the hpm object to refernce this serial stream.
  // Note carefully the '&' in this line.
  hpm.begin(&hpmSerial);

  while ( status != WL_CONNECTED) {
    // Connect to WPA/WPA2 Wi-Fi network
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection
    delay(10000);
  }

  ring.begin();
  ring.setBrightness(25);
  ring.setPixelColor(0, ring.Color(20, 128, 21));
  ring.show(); // initialize LED 1;
  // Wait until HPM is ready
  while (!hpm.isNewDataAvailable()) {
    Serial.println("Still waking up HPM ...");
    delay(1000);
  }
}

// In the loop, we can just poll for new data since the device automatically
// enters auto-send mode on startup.
void loop() {
  if (hpm.isNewDataAvailable()) {

    // get readings
    aqi = hpm.getAQI();
    pm1 = hpm.getPM1();
    pm25 = hpm.getPM25();
    pm4 = hpm.getPM4();
    pm10 = hpm.getPM10();

    // Print to serial
    Serial.print("AQI: ");
    Serial.print(aqi);
    Serial.print("  PM 1 = ");
    Serial.print(pm1);
    Serial.println();
    Serial.print("PM 2.5 = ");
    Serial.print(pm25);
    Serial.print("  PM 4 = ");
    Serial.print(pm4);
    Serial.println();
    Serial.print("  PM 10 = ");
    Serial.print(pm10);
    Serial.println();

    // Only push data and update LEDs after timeout period
    if (millis() - lastConnectionTime > postingInterval) {

      // Update ThingSpeak
      httpRequest();
      Serial.println("Data sent to ThingSpeak");

      // Update LED indicator only if AQI changed
      if (aqi != lastAqi) {
        ledIndicator();
        Serial.println("LED updated");
        lastAqi = aqi;
      }
    }
  }

  // The physical sensor only sends data once per second.
  delay(1000);

}

void httpRequest() {

  // read Wi-Fi signal strength (rssi)
  long rssi = WiFi.RSSI();

  // create data string to send to ThingSpeak
  String data = String("field1=" + String(aqi, DEC) +
                       "&field2=" + String(pm1, DEC) +
                       "&field3=" + String(pm25, DEC) +
                       "&field4=" + String(pm4, DEC) +
                       "&field5=" + String(pm10, DEC) +
                       "&field6=" + String(rssi, DEC));

  // close any connection before sending a new request
  client.stop();

  // POST data to ThingSpeak
  if (client.connect(server, 80)) {
    client.println("POST /update HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("Connection: close");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("X-THINGSPEAKAPIKEY: " + writeAPIKey);
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.print(data.length());
    client.print("\n\n");
    client.print(data);

    // note the last connection time
    lastConnectionTime = millis();
  }
}

// Fill the dots one after the other with a color
void ledIndicator() {

  // Map AQI onto LED range (0-11)
  int aqi11 = map(aqi, 0, 300, 1, 11);

  // Set unused pixels to off
  for (uint16_t i = ring.numPixels() - 1; i > aqi11; i--) {
    ring.setPixelColor(i, ring.Color(0, 0, 0));
  }

  // Step through pixels in reverse order
  for (uint16_t i = aqi11; i > 0; i--) {
    if (i > 7) {
      ring.setPixelColor(i, ring.Color(152, 1, 1));
    } else if (i == 7) {
      ring.setPixelColor(i, ring.Color(255, 0, 0));
    } else if (i > 3) {
      ring.setPixelColor(i, ring.Color(253, 163, 8));
    } else {
      ring.setPixelColor(i, ring.Color(20, 128, 20));
    }
  }

  // Alsways set first pixel (lowest AQI) as on indicator
  ring.setPixelColor(0, ring.Color(20, 128, 20));

  // Update LEDs
  ring.show();
}
