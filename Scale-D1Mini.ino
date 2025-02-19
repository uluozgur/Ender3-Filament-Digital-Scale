/*
  D1 Mini Weight Scale (grams) using HX711 Load Cell module and TM1637 Display
  Revised to suit D1 Mini Author: Ozgur Ulupinar
  Original Author for ATiny85: Jason A. Cox - @jasonacox
  Date: 18 July 2021
  Components:
      D1 Mini Clone
      TM1636 4 Digit 7-Segment LED Display
      Load Cell - 5kg
      HX711 Load Cell Amplifier (ADC)
      5V Power Supply
  Callibration:
      This sketch requires that you calibrate the load cell.  This involves the following steps:
      1) Run the sketch with DEBUG true
      2) Record the "HX711 reading" values with NO load on the scale - this is your "CAL_OFFSET"
      3) Use an trusted scale and weigh an object (grams) - record this value as your "KNOWN_VALUE"
      4) Place the object on the load cell and record the "HX711 reading" - this is "CAL_VALUE"
      5) Compute the CAL_RATIO value = (CAL_VALUE - CAL_OFFSET) / KNOWN_VALUE
      6) Edit the #defines below
      Example:
        CAL_OFFSET = -148550
        KNOWN_VALUE =  382.7186 g
        CAL_VALUE = -107150
        CAL_RATIO = (CAL_VALUE - CAL_OFFSET) / KNOWN_VALUE
        CAL_RATIO = ((-107150) - (-148550 )) / (382.7186)
        CAL_RATIO = 108.17
  Function:
      On start the circuit will read the last TARE value from EEPROM and display the the
      current weight. Press and hold the TARE button and the current weight value will be
      recorded in EEPROM and subtracted from the current reading to "Zero" out the scale.
*/

// Includes
#include "EEPROM.h"   // Used to store tare values in EEPROM
#include "segment.h"  // Animations for Display

// Include TM1637TinyDisplay Library https://github.com/jasonacox/TM1637TinyDisplay
// #include <Arduino.h>
#include <TM1637TinyDisplay.h>

// Include HX711 Arduino Library https://github.com/bogde/HX711
#include <HX711.h>

/*
    CONFIGURATION SECTION - Update these values
*/
// D1 Mini Pin settings - UPDATE this section for your circuit
const int LOADCELL_DOUT_PIN = D5;  // HX711 DT Pin             (D1 Mini: D5)
const int LOADCELL_SCK_PIN = D6;   // HX711 SK Pin             (D1 Mini: D6)
#define CLK D1                     // TM1637 Display CLK Pin   (D1 Mini: D1)
#define DIO D2                     // TM1637 Display DIO Pin   (D1 Mini: D2)
#define BUTTON D7                  // Tare Button - SHOULD BE CONNECTED WITH PULL-UP RESISTOR MIN 10K  (D1 Mini: D7)

// Turn on debug mode for setting callibration
#define DEBUG false

// Calibration values - UPDATE this section for your load cell - see instructions above
#define CAL_OFFSET 59600       // HX711 reading with no weight
#define CAL_VALUE  249800       // HX711 reading with item placed on scale
#define KNOWN_VALUE    482  // True weight of item placed on scale (e.g. in grams)

/*
   Calibration ratio will be computed based on the above
   CAL_RATIO = (CAL_VALUE  -  CAL_OFFSET)  /  KNOWN_VALUE
*/
#define CAL_RATIO  ((CAL_VALUE) - (CAL_OFFSET)) / (KNOWN_VALUE)

#define READ_SAMPLES 15   // Number of readings to average each cycle

/*
   SETUP
*/
// Initialize Global Classes 
TM1637TinyDisplay display(CLK, DIO);
HX711 scale;

void setup() {
#if DEBUG == true
  Serial.begin(38400);
#endif
  pinMode(BUTTON, INPUT);       // Set Tare button
  display.setBrightness(0x0f); // set the brightness to 100 %
  delay(10);

  // Display Startup Sequence
  for (int x = 0; x < 5; x++) {
    display.showString(" On ");
    delay(200);
    display.clear();
    delay(100);
  }
  display.showAnimation(ANIMATION, FRAMES(ANIMATION), TIME_MS(10));
  display.showString("Ender 3 Pro");
  display.showString("----");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  long offset = 0;
  scale.set_scale(CAL_RATIO);
  EEPROM.get(0, offset);
  if (offset == 0) {
    scale.set_offset(CAL_OFFSET); // default
  }
  else {
    scale.set_offset(offset);
  }
delay(100)
}

/*
   LOOP
*/
void loop() {
  if (scale.is_ready()) {
    long reading = scale.read_average(READ_SAMPLES);
#if DEBUG == true
    Serial.print("HX711 reading: ");
    Serial.print(reading);
    Serial.print(" ::: [ CAL_OFFSET: ");
    Serial.print(scale.get_offset());
    Serial.print(", CAL_RATIO: ");
    Serial.print(scale.get_scale());
    Serial.print(", Adjusted Value: ");
    Serial.print((reading - scale.get_offset()) / scale.get_scale());
    Serial.println(" ]");
#endif
    // Display weight
    display.showNumber((int)((reading - scale.get_offset()) / scale.get_scale()));

    // Check to see if TARE button is pushed
    if (digitalRead(BUTTON) == LOW) {
      display.showString("Tare");
      delay(500);
      EEPROM.put(0, reading);       // Save tare value to EEPROM
      scale.set_offset(reading);    // Update offset
      // Display flashing zero to indicate tare set
      for (int x = 0; x < 5; x++) {
        display.clear();
        delay(50);
        display.showNumberDec(0, 4);
        delay(100);
      }
      display.showString("----");
    }
    else {
      delay(100);
    }
  }
  else {
    // HX711 not responding
#if DEBUG == true
    Serial.println("HX711 not found.");
#endif
    display.showString("Err");
    delay(100);
  }
}
