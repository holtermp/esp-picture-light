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
#include <FastLED.h>
#include <pins_arduino.h>

#define LEDPIN    D3
#define LED_TYPE    WS2811
//#define LED_TYPE PL9823
#define COLOR_ORDER GRB
#define NUMLEDS  255

#define BRIGHTNESS_INITIAL  128
#define BRIGHTNESS_MAX  255
#define BRIGHTNESS_MIN  0

u_int8_t currentBrightness  = BRIGHTNESS_INITIAL;

#define RELAY1PIN D5
#define RELAY2PIN D6


#define BRIGHTNESS  128

u_int8_t relay1State = LOW;
u_int8_t relay2State = LOW;

CRGB leds[NUMLEDS];

AsyncWebServer server(80);

const char *ssid = MY_SSID;
const char *password = MY_PW;


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


#define PARAM_UP "up"
#define PARAM_DOWN "down"
#define PARAM_ON "on"
#define PARAM_OFF "off"


void handleRoot(AsyncWebServerRequest *request)
{
  request->send(200, "text/plain", "Welcome to the picture light service.....");
}

void handleNotFound(AsyncWebServerRequest *request)
{
  Serial.println("Not Found:" + request->url());
  request->send(404, "text/plain", "Not found");
}

void handleBrightness(AsyncWebServerRequest *request)
{
  if (request->hasParam(PARAM_ON))
  {
    FastLED.setBrightness(currentBrightness);
    FastLED.show();
    String message = "Set brightness to: ";
    message.concat(currentBrightness);
    message.concat(" saving current brightness");
    Serial.println (message);
    request->send(200, "text/plain", message);
  }else if (request->hasParam(PARAM_OFF))
  {
    FastLED.setBrightness(0);
    FastLED.show();
    String message = "Set brightness to: 0, saving current brightness";
    Serial.println (message);
    request->send(200, "text/plain", message);
  }
  else{
    if (request->hasParam(PARAM_UP) && currentBrightness<BRIGHTNESS_MAX)
    {
      currentBrightness++;
    }
    else if (request->hasParam(PARAM_DOWN) && currentBrightness>BRIGHTNESS_MIN)
    {
      currentBrightness--;
    }
    FastLED.setBrightness(currentBrightness);
    FastLED.show();
    String message = "Set brightness to: ";
    message.concat(currentBrightness);
    Serial.println (message);
    request->send(200, "text/plain", message);
  }
}



void setupServer()
{
  server.on("/", HTTP_GET, handleRoot);
  server.on("/brightness", HTTP_GET, handleBrightness);
  server.onNotFound(handleNotFound);
  server.begin();
}


void setupLights(){
 for (int i = 0; i < NUMLEDS; i++)
  {
    leds[i] = CRGB::Green;
    Serial.println(i);
  }
  FastLED.addLeds<LED_TYPE, LEDPIN,COLOR_ORDER>(leds, sizeof(leds));
}

void writeRelays(){
  digitalWrite(RELAY1PIN,!relay1State);
  digitalWrite(RELAY2PIN,!relay2State);
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY1PIN,OUTPUT);
  pinMode(RELAY2PIN,OUTPUT);
  writeRelays();
  setupWifi();
  setupServer();
  setupLights();
  Serial.println("Setup done");
 } 

void nextRelayState(){

  if(!relay1State && !relay2State){
    relay1State=HIGH;
    relay2State=LOW; 
  }
  else if (relay1State && !relay2State){
    relay1State=LOW;
    relay2State=HIGH; 
  }
  else if (!relay1State && relay2State){
    relay1State=LOW;
    relay2State=LOW; 
  }

  writeRelays();
  
};




void loop() {
  delay(500);
  FastLED.show();
}




