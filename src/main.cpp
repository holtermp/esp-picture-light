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


#define PARAM_STEP "step"
#define PARAM_UP "up"
#define PARAM_DOWN "down"
#define PARAM_ON "on"
#define PARAM_OFF "off"
#define PARAM_TOGGLE "toggle"


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
  if (request->hasParam(PARAM_ON)||request->hasParam(PARAM_OFF)||request->hasParam(PARAM_TOGGLE))
  {
    boolean on = request->hasParam(PARAM_ON);
    if (request->hasParam(PARAM_TOGGLE)){
      on = FastLED.getBrightness()==0;
    }    
    String message = "Set brightness to: ";
    if (on)
    {
      FastLED.setBrightness(currentBrightness);
      message.concat(currentBrightness);
    }else
    {
      FastLED.setBrightness(0);
      message.concat(0);
    }
    Serial.println (message);
    request->send(200, "text/plain", message);
  }
  else{
    uint8_t step=1;
    if (request->hasParam(PARAM_STEP) && currentBrightness+step<=BRIGHTNESS_MAX)
    {
           step = request->getParam(PARAM_STEP)->value().toInt();
    }    
    if (request->hasParam(PARAM_UP) && currentBrightness+step<=BRIGHTNESS_MAX)
    {
      currentBrightness+=step;
    }
    else if (request->hasParam(PARAM_DOWN) && currentBrightness-step>=BRIGHTNESS_MIN)
    {
      currentBrightness-=step;
    }
    FastLED.setBrightness(currentBrightness);
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
    leds[i] = CRGB::White;
  }
  FastLED.addLeds<LED_TYPE, LEDPIN,COLOR_ORDER>(leds, sizeof(leds));
  FastLED.setBrightness(currentBrightness);
  delay (3000);
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
  Serial.print("FPS:");
  Serial.println(FastLED.getFPS());
  
  
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
  delay(100);
  FastLED.show();
}




