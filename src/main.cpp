#include <Arduino.h>
#include <FastLED.h>
#include <pins_arduino.h>

#define LEDPIN    D3
#define LED_TYPE    WS2811
//#define LED_TYPE PL9823
#define COLOR_ORDER GRB
#define NUMLEDS   256
#define BRIGHTNESS  255

#define RELAY1PIN D5
#define RELAY2PIN D6


#define BRIGHTNESS  128
#define FRAMES_PER_SECOND 30
#define COOLING 55
#define SPARKING 120
#define REVERSE_DIRECTION false

u_int8_t relay1State = LOW;
u_int8_t relay2State = LOW;

CRGB leds[NUMLEDS];

void writeRelays(){
  digitalWrite(RELAY1PIN,!relay1State);
  digitalWrite(RELAY2PIN,!relay2State);
}



void setup() {
  Serial.begin(115200);
  pinMode(RELAY1PIN,OUTPUT);
  pinMode(RELAY2PIN,OUTPUT);
  writeRelays();
 for (int i = 0; i < NUMLEDS; i++)
  {
    leds[i] = CRGB (128,128,128);
  }
  delay(3000);
  FastLED.addLeds<LED_TYPE, LEDPIN,COLOR_ORDER>(leds, sizeof(leds));
  randomSeed(analogRead(0));
  Serial.println("Setup done");
 } 
 



int currentLed = 0;

#define MIN_COL 100
#define MAX_COL 255

uint8_t change(uint8_t color) {
  if (color==MAX_COL || color==MIN_COL) {
    return color;
  }
  return (u_int8_t) (color+(int8)random(-1,2));
}

void nextLedState() {

  for (int i = 0; i < NUMLEDS; i++)
  {
   leds[i] = CRGB(change(leds[i].red),change(leds[i].green),change(leds[i].blue));
  }
  Serial.print("nextLedState:");
  Serial.print(leds[0].red);
  Serial.print(",");
  Serial.print(leds[0].green);
  Serial.print(",");
  Serial.print(leds[0].blue);
  Serial.println("");
  
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
 
 FastLED.show();
 delay(100);
  nextLedState();
}




