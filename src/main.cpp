#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <pins_arduino.h>
#include "wifi_secrets.h"
#include <Adafruit_NeoPixel.h>
#include <pins_arduino.h>

#define LED_PIN D3
// DELETEIFNOLONGERNEEDED #define LED_TYPE    WS2811
// #define LED_TYPE PL9823

#define NUMLEDS 255

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(NUMLEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

uint32_t colorStore[NUMLEDS];

#define BRIGHTNESS_INITIAL 64
#define BRIGHTNESS_MAX 255
#define BRIGHTNESS_MIN 0

#define HUE_INITIAL 2000
#define SAT_INITIAL 128

u_int16_t currentHue = HUE_INITIAL;
u_int8_t currentSat = SAT_INITIAL;
u_int8_t currentBrightness = BRIGHTNESS_INITIAL;

#define RELAY1PIN D5
#define RELAY2PIN D6

u_int8_t relay1State = LOW;
u_int8_t relay2State = LOW;

AsyncWebServer server(80);

const char *ssid = MY_SSID;
const char *password = MY_PW;

boolean autoHue = false;
boolean autoSat = false;

#define AUTO_HUE_CHANGE_STEP 350
#define AUTO_SAT_CHANGE_STEP 8

void setupWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  Serial.println("IP:");
  WiFi.localIP().printTo(Serial);
  Serial.println();
}

#define PARAM_STEP "step"
#define PARAM_UP "up"
#define PARAM_DOWN "down"
#define PARAM_ON "on"
#define PARAM_OFF "off"
#define PARAM_TOGGLE "toggle"
#define PARAM_AUTO "auto"

void handleRoot(AsyncWebServerRequest *request)
{
  request->send(200, "text/plain", "Welcome to the picture light service.....");
}

void handleNotFound(AsyncWebServerRequest *request)
{
  Serial.println("Not Found:" + request->url());
  request->send(404, "text/plain", "Not found");
}

void saveCurrentColors()
{
  for (int c = 0; c < NUMLEDS; c++)
  {
    colorStore[c] = strip.getPixelColor(c);
  }
}

void restoreSavedColors()
{
  for (int c = 0; c < NUMLEDS; c++)
  {
    strip.setPixelColor(c, colorStore[c]);
  }
}

void updateHueSat()
{
  String message = "Hue, Sat, Val: ";
  message.concat(currentHue);
  message.concat(",");
  message.concat(currentSat);
  message.concat(",");
  message.concat(currentBrightness);
  Serial.println(message);
  for (int c = 0; c < NUMLEDS; c++)
  {
    strip.setPixelColor(c, Adafruit_NeoPixel::ColorHSV(currentHue, currentSat, currentBrightness));
  }
}

void handleBrightness(AsyncWebServerRequest *request)
{
  if (request->hasParam(PARAM_ON) || request->hasParam(PARAM_OFF) || request->hasParam(PARAM_TOGGLE))
  {
    boolean on = request->hasParam(PARAM_ON);
    if (request->hasParam(PARAM_TOGGLE))
    {
      on = strip.getBrightness() == 0;
    }
    String message = "Set brightness to: ";
    if (on)
    {
      restoreSavedColors();
      strip.setBrightness(currentBrightness);
      message.concat(currentBrightness);
    }
    else
    {
      saveCurrentColors();
      strip.setBrightness(0);
      message.concat(0);
    }
    Serial.println(message);
    request->send(200, "text/plain", message);
  }
  else
  {
    uint8_t step = 1;
    if (request->hasParam(PARAM_STEP) && currentBrightness + step <= BRIGHTNESS_MAX)
    {
      step = request->getParam(PARAM_STEP)->value().toInt();
    }
    if (request->hasParam(PARAM_UP) && currentBrightness + step <= BRIGHTNESS_MAX)
    {
      currentBrightness += step;
    }
    else if (request->hasParam(PARAM_DOWN) && currentBrightness - step >= BRIGHTNESS_MIN)
    {
      currentBrightness -= step;
    }
    strip.setBrightness(currentBrightness);
    String message = "Set brightness to: ";
    message.concat(currentBrightness);
    Serial.println(message);
    request->send(200, "text/plain", message);
  }
  strip.show();
}

String toString(boolean b)
{
  return b ? "true" : "false";
}

void handleHue(AsyncWebServerRequest *request)
{
  String message = "";
  if (request->hasParam(PARAM_AUTO))
  {
    autoHue = request->getParam(PARAM_AUTO)->value().compareTo("true") == 0;
    message = ("Set auto hue to:");
    message.concat(toString(autoHue));
  }
  else
  {
    uint8_t step = 1;

    if (request->hasParam(PARAM_STEP))
    {
      step = request->getParam(PARAM_STEP)->value().toInt();
    }
    if (request->hasParam(PARAM_UP))
    {
      currentHue += step;
    }
    else if (request->hasParam(PARAM_DOWN))
    {
      currentHue -= step;
    }
    updateHueSat();
    message = "Set hue to: ";
    message.concat(currentHue);
  }
  Serial.println(message);
  request->send(200, "text/plain", message);
  strip.show();
}

void handleSat(AsyncWebServerRequest *request)
{
  String message = "";
  if (request->hasParam(PARAM_AUTO))
  {
    autoSat = request->getParam(PARAM_AUTO)->value().compareTo("true") == 0;
    message = ("Set auto sat to:");
    message.concat(toString(autoSat));
  }
  else
  {
    uint8_t step = 1;
    if (request->hasParam(PARAM_STEP))
    {
      step = request->getParam(PARAM_STEP)->value().toInt();
    }
    if (request->hasParam(PARAM_UP))
    {
      currentSat += step;
    }
    else if (request->hasParam(PARAM_DOWN))
    {
      currentHue -= step;
    }
    updateHueSat();
    message = "Set sat to: ";
    message.concat(currentSat);
  }
  Serial.println(message);
  request->send(200, "text/plain", message);
  strip.show();
}

void setupServer()
{
  server.on("/", HTTP_GET, handleRoot);
  server.on("/brightness", HTTP_GET, handleBrightness);
  server.on("/hue", HTTP_GET, handleHue);
  server.on("/sat", HTTP_GET, handleSat);
  server.onNotFound(handleNotFound);
  server.begin();
}

void setupLights()
{
  strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();  // Turn OFF all pixels ASAP
  strip.clear();
  strip.setBrightness(currentBrightness);
  updateHueSat();
  strip.show();
}

void writeRelays()
{
  digitalWrite(RELAY1PIN, !relay1State);
  digitalWrite(RELAY2PIN, !relay2State);
}

void setup()
{
  Serial.begin(115200);
  pinMode(RELAY1PIN, OUTPUT);
  pinMode(RELAY2PIN, OUTPUT);
  writeRelays();
  setupWifi();
  setupServer();
  setupLights();
  Serial.println("Setup done");
}

void nextRelayState()
{

  if (!relay1State && !relay2State)
  {
    relay1State = HIGH;
    relay2State = LOW;
  }
  else if (relay1State && !relay2State)
  {
    relay1State = LOW;
    relay2State = HIGH;
  }
  else if (!relay1State && relay2State)
  {
    relay1State = LOW;
    relay2State = LOW;
  }

  writeRelays();
};

void loop()
{
  delay(100);
  if (autoHue || autoSat)
  {
    if (autoHue)
    {
      currentHue += AUTO_HUE_CHANGE_STEP;
    }
    if (autoSat)
    {
      currentSat += AUTO_SAT_CHANGE_STEP;
    }
    updateHueSat();
    strip.show();
  }
}
