/*
  05.LoopTemplateFun

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
/* Differences from 04.LoopTemplate
 *
 * This sketch addresses the downside of using static template
 * class functions, which is that some folk may view the source
 * code as less readable.
 * 
 * To address this, this sketch uses a C++11 capability to 
 * generate a shorter alias for the template functions:
 *     QDec<PIN_A, PIN_B, uint32_t useFullStep = 0>::begin()
 *     QDec<PIN_A, PIN_B, uint32_t useFullStep = 0>::update()
 *
 * C++11 is supported by current Arduino Nano boards, and is 
 * likely to work with most boards, because C++11 support is
 * now extremely common.
 *
 * This is **strictly optional**, as it only makes the code
 * easier to read, without changing how it functions.
 *
 * When done, move to the next example.
 */

#include "Arduino.h"
#include "qdec.h"

const int ROTARY_PIN_A = 3; // the first pin connected to the rotary encoder
const int ROTARY_PIN_B = 2; // the second pin connected to the rotary encoder

// No class instance declared ... not required when using templated version
// but let's define two aliases so don't need to type so much
const auto& myQdecBegin  = ::SimpleHacks::QDec<ROTARY_PIN_A, ROTARY_PIN_B, true>::begin;
const auto& myQdecUpdate = ::SimpleHacks::QDec<ROTARY_PIN_A, ROTARY_PIN_B, true>::update;

// Stores a relative value based on the clockwise / counterclockwise events
int rotaryCount = 0;

void setup() {
  delay(2000);
  
  Serial.begin(115200);
  while (!Serial) {} // wait for serial port to connect... needed for boards with native USB serial support
  Serial.print("Beginning QDecoder Sketch ");  
  Serial.println(__FILE__); // becomes the sketch's filename during compilation

  // initialize the rotary encoder
  myQdecBegin(); // use the C++ alias generated above for better readability
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
  ::SimpleHacks::QDECODER_EVENT event = myQdecUpdate();  // use the C++ alias generated above for better readability

  // increment for clockwise, decrement for counter-clockwise
  if (event & ::SimpleHacks::QDECODER_EVENT_CW) {
    rotaryCount = rotaryCount + 1;
    Serial.print("change: "); Serial.println(rotaryCount);
  } else if (event & ::SimpleHacks::QDECODER_EVENT_CCW) {
    rotaryCount = rotaryCount - 1;
    Serial.print("change: "); Serial.println(rotaryCount);
  }
} // end of void loop()
