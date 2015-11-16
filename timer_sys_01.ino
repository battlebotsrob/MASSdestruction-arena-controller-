/*
 Robot Arena Match Timer 
 By:  Andrew Anselmo Featuring Robert Masek
 MASSdestruction
 Date: November 15th, 2015
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 Original code came from Sparkfun https://learn.sparkfun.com/tutorials/large-digit-driver-hookup-guide
 SOme code for the seven segment display came from here https://learn.adafruit.com/adafruit-led-backpack/1-2-inch-7-segment-backpack  

 Adafruit Adafruit 1.2" 4-Digit 7-Segment Display w/I2C Backpack - Yellow  1269 http://www.adafruit.com/product/1269  
 Sparkfun SparkFun Large Digit Driver WIG-13279 https://www.sparkfun.com/products/13279?_ga=1.220724176.97185491.1447045863
 Sparkfun 7-Segment Display - 6.5" (Red) COM-08530  https://www.sparkfun.com/products/8530?_ga=1.220724176.97185491.1447045863
 
 Code to for timing system; Rob and Andrew; Artisan's Asylum
          - This drives a big seven segment display AND a smaller I2C system
          

 

*/

#define VERSIONID 20151115


#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
//#include <TinyWireM.h> // Enable this line if using Adafruit Trinket, Gemma, etc.

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

Adafruit_7segment matrix = Adafruit_7segment();

//GPIO declarations
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
byte segmentClock = 6;
byte segmentLatch = 5;
byte segmentData = 7;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


#define SET_TWO_MIN 8
#define SET_THREE_MIN 9
#define START_MATCH 10
#define PAUSE_MATCH 11

void setup()
{
  Serial.begin(9600);
  Serial.println("Timer System for Artisan's Asylum Robot Arena");
  Serial.print("Version = ");
  Serial.println(VERSIONID);

  pinMode(segmentClock, OUTPUT);
  pinMode(segmentData, OUTPUT);
  pinMode(segmentLatch, OUTPUT);

  digitalWrite(segmentClock, LOW);
  digitalWrite(segmentData, LOW);
  digitalWrite(segmentLatch, LOW);

  pinMode(SET_TWO_MIN, INPUT_PULLUP);
  pinMode(SET_THREE_MIN, INPUT_PULLUP);
  pinMode(START_MATCH, INPUT_PULLUP);
  pinMode(PAUSE_MATCH, INPUT_PULLUP);

  // seven segment display I2C, default address
  matrix.begin(0x70);
    

}

int number = 0;
unsigned long CurrentTimeMillis;
unsigned long StartTimeMillis;
unsigned long NetTimeMillis;
unsigned long MatchLengthSeconds;
unsigned int TimeLeftSeconds = 999;
boolean Started = false;

boolean TwoMinInput;
boolean ThreeMinInput;
boolean StartInput;
boolean PauseInput;

void loop()
//////////////////////////////////////////////////////////////////////
{

  TwoMinInput = digitalRead(SET_TWO_MIN);
  ThreeMinInput = digitalRead(SET_THREE_MIN);
  StartInput = digitalRead(START_MATCH);
  PauseInput = digitalRead(PAUSE_MATCH);

  CurrentTimeMillis = millis();

  /* test code
  showNumber(number); //Test pattern
  number++;
  number %= 1000; //Reset x after 99

  Serial.println(number); //For debugging

  delay(100);
  */

  // Setting match length, 2 minutes
  if (TwoMinInput == LOW && Started == false)
  {
    MatchLengthSeconds = 120;
    TimeLeftSeconds = MatchLengthSeconds;
  }

  // Setting match length, 3 minutes
  if (ThreeMinInput == LOW && Started == false)
  {
    MatchLengthSeconds = 180;
    TimeLeftSeconds = MatchLengthSeconds;
  }

  // starting (or unpausing) the match
  if (StartInput == LOW && Started == false)
  {
    Serial.println("Starting...");
    StartTimeMillis = CurrentTimeMillis;
    Started = true;
  }

  // pausing the match, when started
  if(PauseInput==LOW && Started==true)
  {
    MatchLengthSeconds=TimeLeftSeconds;
    Started=false;
  }

  // if started, count down...
  if (Started == true)
  {
    NetTimeMillis = CurrentTimeMillis - StartTimeMillis;
    TimeLeftSeconds = int(MatchLengthSeconds - NetTimeMillis * 0.001);

    if (TimeLeftSeconds <= 0)
    {
      Started = false;
      TimeLeftSeconds = 0;
    }

  }

  ShowAllNumbers(TimeLeftSeconds);

  /*
  Serial.print("Input = ");
  Serial.print(TwoMinInput);
  Serial.print(" ");
  Serial.print("Countdown; total = ");
  Serial.print(MatchLengthSeconds);
  Serial.print(" ");
  Serial.print(TimeLeftSeconds);
  Serial.print("\n");
  */

}

void ShowAllNumbers(int value)
//////////////////////////////////////////////////////////////////////
{

    // always show a number
  showNumber(value);
  matrix.print(value, DEC);
  matrix.writeDisplay();
  
  
}

//Takes a number and displays 2 numbers. Displays absolute value (no negatives)
void showNumber(float value)
//////////////////////////////////////////////////////////////////////
{
  int number = abs(value); //Remove negative signs and any decimals

  //Serial.print("number: ");
  //Serial.println(number);

  for (byte x = 0 ; x < 3 ; x++)
  {
    int remainder = number % 10;

    postNumber(remainder, false);

    number /= 10;
  }

  //Latch the current segment data
  digitalWrite(segmentLatch, LOW);
  digitalWrite(segmentLatch, HIGH); //Register moves storage register on the rising edge of RCK
}

//Given a number, or '-', shifts it out to the display
void postNumber(byte number, boolean decimal)
{
  //    -  A
  //   / / F/B
  //    -  G
  //   / / E/C
  //    -. D/DP

#define a  1<<0
#define b  1<<6
#define c  1<<5
#define d  1<<4
#define e  1<<3
#define f  1<<1
#define g  1<<2
#define dp 1<<7

  byte segments;

  switch (number)
  {
    case 1: segments = b | c; break;
    case 2: segments = a | b | d | e | g; break;
    case 3: segments = a | b | c | d | g; break;
    case 4: segments = f | g | b | c; break;
    case 5: segments = a | f | g | c | d; break;
    case 6: segments = a | f | g | e | c | d; break;
    case 7: segments = a | b | c; break;
    case 8: segments = a | b | c | d | e | f | g; break;
    case 9: segments = a | b | c | d | f | g; break;
    case 0: segments = a | b | c | d | e | f; break;
    case ' ': segments = 0; break;
    case 'c': segments = g | e | d; break;
    case '-': segments = g; break;
  }

  if (decimal) segments |= dp;

  //Clock these bits out to the drivers
  for (byte x = 0 ; x < 8 ; x++)
  {
    digitalWrite(segmentClock, LOW);
    digitalWrite(segmentData, segments & 1 << (7 - x));
    digitalWrite(segmentClock, HIGH); //Data transfers to the register on the rising edge of SRCK
  }
}
