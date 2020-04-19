/*
  03.LoopNamespace

  This uses the SimpleHacks Quadrature Decoder (aka rotary decoder) library
  to obtain clockwise / counterclockwise events.
  
  This sketch uses the QDecoder class, and simply spins calling update()
  as quickly as possible, and reacts when an event is reported. 
  A clockwise event will increase a counter varaible,
  while a counter-clockwise event will decrease that counter variable.

  The circuit requires the following:
  * an arduino with two digital input pins (here, using A3 and A5)
  * a rotary encoder
    * pin A and pin B connect to Arduino pins 2 and 3    
    * the third pin (sometimes designated pin C) connects to ground (GND)
  See circuit diagram at: https://github.com/SimpleHacks/QDEC/docs/

  Created 2019-09-29
  By Henry Gabryjelski
*/
/* Difference from 02.LoopLearn
 *  
 * Rather than typing the fully-qualified name, such as:
 *     ::SimpleHacks::QDecoder(PIN_B, PIN_B, true);
 *     if (event & ::SimpleHacks::QDECODER_EVENT_CW) {
 *
 * By adding the following line at the top of the file:
 *     using namespace ::SimpleHacks;
 *
 * The above examples can be simplified to:
 *     QDecoder(PIN_B, PIN_B, true);
 *     if (event & QDECODER_EVENT_CW) {
 *
 * Move to the next example.
 */

#include "Arduino.h"
#include "qdec.h"

using namespace ::SimpleHacks;

const int ROTARY_PIN_A = 3; // the first pin connected to the rotary encoder
const int ROTARY_PIN_B = 2; // the second pin connected to the rotary encoder

QDecoder qdec(ROTARY_PIN_A, ROTARY_PIN_B, true); // the library class...

// Stores a relative value based on the clockwise / counterclockwise events
int rotaryCount = 0;

void setup() {
  delay(2000);
  
  Serial.begin(115200);
  while (!Serial) {} // wait for serial port to connect... needed for boards with native USB serial support
  Serial.print("Beginning QDecoder Sketch ");  
  Serial.println(__FILE__); // becomes the sketch's filename during compilation

  // initialize the rotary encoder
  qdec.begin();
}

void DoFakeTask(void) {
  // Simulate work that takes significant processing time
  unsigned long millisecondsToDelay = 0;
  //millisecondsToDelay = 5;
  //millisecondsToDelay = 1000;
  delay(millisecondsToDelay);
}

void loop() {
  DoFakeTask();

  // poll the state of the quadrature encoder,
  // and storing the result (which is an event, if one occurred this time)
  QDECODER_EVENT event = qdec.update();

  // increment for clockwise, decrement for counter-clockwise
  if (event & QDECODER_EVENT_CW) {
    rotaryCount = rotaryCount + 1;
    Serial.print("change: "); Serial.println(rotaryCount);
  } else if (event & QDECODER_EVENT_CCW) {
    rotaryCount = rotaryCount - 1;
    Serial.print("change: "); Serial.println(rotaryCount);
  }
} // end of void loop()
