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
#include <UniversalTelegramBot.h>

// Setup
#define USE_LED
#define USE_WIFI

#ifdef USE_WIFI
  // WiFi settings
  char ssid[] = SECRET_SSID; //  your network SSID (name)
  char pass[] = SECRET_PASS; // your network password
  int status = WL_IDLE_STATUS;
  
  // ThingSpeak Settings
  char server[] = "api.thingspeak.com";
  String writeAPIKey = SECRET_API_KEY;
  unsigned long lastConnectionTime = 0; // track the last connection time
  const unsigned long postingInterval = 60L * 1000L; // post data every 60 seconds

  // WiFi Client
  WiFiSSLClient client;

  // Telegram bot
  UniversalTelegramBot bot(BOT_TOKEN, client);  
  unsigned long botLastTime = 0;
  const unsigned long botPostingInterval = 24L * 60L * 60L * 1000L; // post data once every day
#endif

// HPMA settings and variables
// Connect HPMA to Nano pin 1 and 2 (Serial1 RX and TX)
uint16_t pm1, pm25, pm4, pm10, aqi, lastAqi;
unsigned long pm1Cum, pm25Cum, pm4Cum, pm10Cum, aqiCum;
uint16_t loopCount = 0;

#ifdef USE_LED
  // Neopixel settigns
  #define LEDPIN 2
  #define NUMPIXELS 12
#endif

int aqi11;

#ifdef USE_LED
  // Initialize NeoPixel ring
  Adafruit_NeoPixel ring = Adafruit_NeoPixel(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);
#endif

// Create an object to communicate with the HPM Compact sensor.
HPMA115_Compact hpm = HPMA115_Compact();

void setup() {
  
  // Console serial.
  Serial.begin(115200);
  Serial.println("Serial initialized");

  // Serial for ineracting with HPM device.
  Serial1.begin(HPMA115_BAUD);
  Serial.println("HPMA serial initialized");

  // Configure the hpm object to refernce this serial stream.
  // Note carefully the '&' in this line.
  hpm.begin(&Serial1);

  #ifdef USE_LED
    ring.begin();
    ring.setBrightness(25);
    ring.setPixelColor(0, ring.Color(20, 128, 21));
    ring.show(); // initialize LED 1;
  #endif
  
  #ifdef USE_WIFI
    // Connect to WPA/WPA2 Wi-Fi network
    Serial.println("Connecting Wifi");
      
    while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
  
      // wait 1 second for connection
      rainbowCycle(10);
      Serial.print(".");
    }
  #endif
  // Wait until HPM is ready
  Serial.println("Waking up HPM ...");
  while (!hpm.isNewDataAvailable()) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("All set");

}

// In the loop, we can just poll for new data since the device automatically
// enters auto-send mode on startup. Postings are nested, so we don't have to 
// run all the checks in every iteration.
void loop() {
  if (hpm.isNewDataAvailable()) {
    // Increment counter
    loopCount++;

    // get readings
    aqiCum += hpm.getAQI();
    pm1Cum += hpm.getPM1();
    pm25Cum += hpm.getPM25();
    pm4Cum += hpm.getPM4();
    pm10Cum += hpm.getPM10();

    aqi = aqiCum / loopCount;
    pm1 = pm1Cum / loopCount;
    pm25 = pm25Cum / loopCount;
    pm4 = pm4Cum / loopCount;
    pm10 = pm10Cum / loopCount;

    aqi11 = map(aqi, 1, 500, 1, 11);

    // Print to serial
    Serial.print("Iteration: ");
    Serial.print(loopCount);
    Serial.print("  AQI: ");
    Serial.print(aqi);
    Serial.print("  PM 1 = ");
    Serial.print(pm1);
    Serial.print("  PM 2.5 = ");
    Serial.print(pm25);
    Serial.print("  PM 4 = ");
    Serial.print(pm4);
    Serial.print("  PM 10 = ");
    Serial.print(pm10);
    Serial.print(" LED = ");
    Serial.print(aqi11);
    Serial.println();

    
    // Only push data and update LEDs after timeout period
    if (millis() - lastConnectionTime > postingInterval) {
      #ifdef USE_WIFI
        // Update ThingSpeak
        httpRequest();
        Serial.println("Data sent to ThingSpeak");
    
        // Only post to Telegram once in 24h. 
        char message[12];
        sprintf(message, "AQI: %d", aqi);
        if(millis() - botLastTime > botPostingInterval) {
          bot.sendMessage(USER_ID, message, "");
        }
      #endif
  
      // Reset counter and cumulative variables
      loopCount = aqiCum = pm1Cum = pm25Cum = pm4Cum = pm10Cum = 0;
      lastConnectionTime = millis();
    }

    // Update LED indicator only if AQI changed
    #ifdef USE_LED
      if (aqi11 != lastAqi) {
        ledIndicator();
        Serial.println("LED updated");
        lastAqi = aqi11;
      }
    #endif
  } else {
    Serial.println("No new data");
  }

  // The physical sensor only sends data once per second.
  delay(2000);

}

#ifdef USE_WIFI
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
#endif

#ifdef USE_LED
// Fill the dots one after the other with a color
void ledIndicator() {

  // Set unused pixels to off
  for (int i = ring.numPixels() - 1; i == aqi11; i--) {
    ring.setPixelColor(i, ring.Color(0, 0, 0));
  }

  // Step through pixels in reverse order
  for (uint16_t i = aqi11-1; i > 0; i--) {
    Serial.print(i);
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

  // Always set first pixel (lowest AQI) as on indicator
  ring.setPixelColor(0, ring.Color(20, 128, 20));

  // Update LEDs
  ring.show();
}

void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< ring.numPixels(); i++) {
      ring.setPixelColor(i, Wheel(((i * 256 / ring.numPixels()) + j) & 255));
    }
    ring.show();
    delay(wait);
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return ring.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return ring.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return ring.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
#endif
