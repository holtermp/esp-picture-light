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
#include <NTPClient.h>
#include <WiFiUdp.h>

#define LED_PIN D3
#define NUMLEDS 256

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(NUMLEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

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

#define AUTO_HUE_CHANGE_STEP 100
#define AUTO_SAT_CHANGE_STEP 2
#define PULSE_ON_STARTUP true

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
#define PARAM_VALUE "value"
String startupTime = "";

void handleRoot(AsyncWebServerRequest *request)
{
  String msg = "Welcome to the picture light service.....\n";
  msg += "Up since: " + startupTime;
  request->send(200, "text/plain", msg);
}

void handleNotFound(AsyncWebServerRequest *request)
{
  Serial.println("Not Found:" + request->url());
  request->send(404, "text/plain", "Not found");
}

void saveCurrentColors()
{
  for (u_int16_t c = 0; c < strip.numPixels(); c++)
  {
    colorStore[c] = strip.getPixelColor(c);
  }
}

void restoreSavedColors()
{
  for (u_int16_t c = 0; c < strip.numPixels(); c++)
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

void handleAbsoluteBrightness(AsyncWebServerRequest *request, String message)
{
  strip.setBrightness(currentBrightness);
  message.concat("Set brightness to: ");
  message.concat(currentBrightness);
  message.concat("\n");
  Serial.print(message);
  request->send(200, "text/plain", message);
}

String toString(boolean b)
{
  return b ? "true" : "false";
}

boolean lampOn = false;

void handleBrightness(AsyncWebServerRequest *request)
{
  if (request->hasParam(PARAM_ON) || request->hasParam(PARAM_OFF) || request->hasParam(PARAM_TOGGLE))
  {
    if (request->hasParam(PARAM_ON))
    {
      lampOn = true;
    }
    else if (request->hasParam(PARAM_OFF))
    {
      lampOn = false;
    }
    else if (request->hasParam(PARAM_TOGGLE))
    {
      lampOn = strip.getBrightness() == 0;
    }
    String message = "lampOn:";
    message += toString(lampOn);
    message += "\nSet brightness to: ";
    if (lampOn)
    {
      restoreSavedColors();
      strip.setBrightness(currentBrightness);
      updateHueSat();
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
  else if (request->hasParam(PARAM_VALUE))
  {
    int value = request->getParam(PARAM_VALUE)->value().toInt();
    String message = "";

    if (value > BRIGHTNESS_MAX)
    {
      message.concat("Requested value to high:");
      message.concat(value);
      message.concat(", set to: ");
      message.concat(BRIGHTNESS_MAX);
      message.concat("\n");
      value = BRIGHTNESS_MAX;
    }
    else if (value < BRIGHTNESS_MIN)
    {

      message.concat("Requested value to low:");
      message.concat(value);
      message.concat(", set to: ");
      message.concat(BRIGHTNESS_MIN);
      message.concat("\n");
      value = BRIGHTNESS_MIN;
    }

    currentBrightness = value;

    handleAbsoluteBrightness(request, message);
  }
  else
  {
    uint8_t step = 1;
    String msg = "";
    if (request->hasParam(PARAM_STEP))
    {
      step = request->getParam(PARAM_STEP)->value().toInt();
      msg += "Step:";
      msg += step;
    }
    if (request->hasParam(PARAM_UP) && currentBrightness + step <= BRIGHTNESS_MAX)
    {
      currentBrightness += step;
    }
    else if (request->hasParam(PARAM_DOWN) && currentBrightness - step >= BRIGHTNESS_MIN)
    {
      currentBrightness -= step;
    }
    handleAbsoluteBrightness(request, "");
  }
  strip.show();
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

boolean pulse = PULSE_ON_STARTUP;

void handlePulse(AsyncWebServerRequest *request)
{
  String message = "";
  if (request->hasParam(PARAM_ON))
  {
    message = ("Set pulse to:true");
    pulse = true;
  }
  else if (request->hasParam(PARAM_OFF))
  {
    message = ("Set pulse to:false");
    pulse = false;
  }
  else if (request->hasParam(PARAM_TOGGLE))
  {
    pulse = !pulse;
    message = ("Toggled pulse to:");
    message.concat(pulse);
  }
  Serial.println(message);
  request->send(200, "text/plain", message);
  strip.show();
}

#define RELAY_RUN_TIME_MS 45000
long relayTimeLeftMs = 0L;

void writeRelays()
{
  digitalWrite(RELAY1PIN, !relay1State);
  digitalWrite(RELAY2PIN, !relay2State);
}

void handleScreenRequest(AsyncWebServerRequest *request)
{
  String message = "";
  if (request->hasParam(PARAM_UP) || request->hasParam(PARAM_DOWN))
  {

    if (request->hasParam(PARAM_UP))
    {
      if (relay1State != HIGH)
        relayTimeLeftMs = RELAY_RUN_TIME_MS;
      message.concat("Setting relay to high: UP");
      relay1State = HIGH;
      relay2State = LOW;
    }
    else if (request->hasParam(PARAM_DOWN))
    {
      if (relay2State != HIGH)
        relayTimeLeftMs = RELAY_RUN_TIME_MS;
      message.concat("Setting relay to high: DOWN");
      relay1State = LOW;
      relay2State = HIGH;
    }
    message.concat(", time left (ms) ");
    message.concat(relayTimeLeftMs);
    writeRelays();
  }
  else if (request->hasParam(PARAM_OFF))
  {
    message.concat("Switching relays off ");
    relayTimeLeftMs = 0;
    relay1State = LOW;
    relay2State = LOW;
    writeRelays();
  }
  else
  {
    message.concat("Did not find relay Paramater UP, DOWN or OFF");
    message.concat(relayTimeLeftMs);
  }
  Serial.println(message);
  request->send(200, "text/plain", message);
}

void setupServer()
{
  server.on("/", HTTP_GET, handleRoot);
  server.on("/brightness", HTTP_GET, handleBrightness);
  server.on("/hue", HTTP_GET, handleHue);
  server.on("/sat", HTTP_GET, handleSat);
  server.on("/pulse", HTTP_GET, handlePulse);
  server.on("/screen", HTTP_GET, handleScreenRequest);
  server.onNotFound(handleNotFound);
  server.begin();
}

void startupTest()
{
  uint32_t colors[] = {Adafruit_NeoPixel::Color(255, 0, 0), Adafruit_NeoPixel::Color(0, 255, 0), Adafruit_NeoPixel::Color(0, 0, 255)};
  for (int i = 0; i < NUMLEDS; i++)
  {
    strip.clear();
    strip.setPixelColor(i, colors[i % 3]);
    strip.show();
    delay(5);
  }
  for (int i = NUMLEDS - 1; i >= 0; i--)
  {
    strip.clear();
    strip.setPixelColor(i, colors[i % 3]);
    strip.show();
    delay(5);
  }
}
void setupLights()
{
  strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  startupTest();
  strip.setBrightness(0);
  strip.clear();
  strip.show(); // Turn OFF all pixels ASAP
}

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void registerStartTime()
{
  timeClient.begin();
  timeClient.update();
  startupTime = weekdays[timeClient.getDay() % 7];
  startupTime += ", ";
  startupTime += timeClient.getFormattedTime();
  startupTime += " (UTC))";
  timeClient.end();
};

void setup()
{
  Serial.begin(115200);
  pinMode(RELAY1PIN, OUTPUT);
  pinMode(RELAY2PIN, OUTPUT);
  writeRelays();
  setupWifi();
  setupServer();
  setupLights();
  registerStartTime();
  Serial.println("Setup done");
  Serial.println(startupTime);
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

#define DELAY_TIME_MS 25
void handleRelays()
{
  if (relayTimeLeftMs > 0)
  {
    relayTimeLeftMs -= DELAY_TIME_MS;
    if (relay1State == HIGH || relay2State == HIGH)
    {
      if (relayTimeLeftMs <= 0)
      {
        Serial.println("Switching off relays");
        relay1State = LOW;
        relay2State = LOW;
        writeRelays();
      }
    }
  }
}

uint8 pulseCounter = 0;

#define SCALE_FACTOR 0.2f
#define SIN_STRETCH_FACTOR 0.05f
#define COS_STRETCH_FACTOR -0.13f

void xp_setBrightness()
{

  for (u_int16_t c = 0; c < strip.numPixels(); c++)
  {
    uint8_t mod_b = currentBrightness + currentBrightness * SCALE_FACTOR * sin(((float)(c + pulseCounter) * SIN_STRETCH_FACTOR)) + currentBrightness * SCALE_FACTOR * cos(((float)(c + pulseCounter) * COS_STRETCH_FACTOR));
    strip.setPixelColor(c, Adafruit_NeoPixel::ColorHSV(currentHue, currentSat, mod_b));
  }
}

void xp_setSat()
{

  for (u_int16_t c = 0; c < strip.numPixels(); c++)
  {
    uint8_t mod_sat = currentSat + currentSat * SCALE_FACTOR * sin(((float)(c + pulseCounter) * SIN_STRETCH_FACTOR)) + currentSat * SCALE_FACTOR * cos(((float)(c + pulseCounter) * COS_STRETCH_FACTOR));
    strip.setPixelColor(c, Adafruit_NeoPixel::ColorHSV(currentHue, mod_sat, currentBrightness));
  }
}

boolean up = true;

void modPulseCounter()
{

  if (up && pulseCounter < 255)
  {
    pulseCounter++;
  }
  else
  {
    up = false;
    pulseCounter--;
    if (pulseCounter < 1)
    {
      up = true;
    }
  }
}

void loop()
{
  modPulseCounter();
  if (pulse)
  {
    xp_setBrightness();
    strip.show();
  }

  delay(DELAY_TIME_MS);
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
  handleRelays();
}
