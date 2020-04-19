/*
  02.LoopLearn

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
/* Learn by doing:
 *  
 * NOTE: Best results when using a rotary encoder with detents (clicks).
 *       If your encoder doesn't have detents, then estimate the amount
 *       required for a single event, or use a full rotation of the knob
 *       as a single detent.
 *
 * 1. Upload the sketch, and enable the serial monitor
 *    (Tools -> Serial Monitor)
 * 2. Rotate the encoder and see how much the value changes per
 *    detent (click).  For example, it's likely going to change
 *    either 1 or 2 values per click.  Note which direction
 *    (clockwise or anti-clockwise) increases the value, and
 *    which decreases the value.
 * 3. Swap which pins are defined as ROTARY_PIN_A and
 *    ROTARY_PIN_B, re-upload the sketch. 
 *    What is different when you rotate the decoder now?
 *    (Revert this change as desired.)
 * 4. In function DoFakeTask(), increase the delay to
 *    5 milliseconds, and re-upload the sketch.
 *    Rotate the encoder quickly, and then rotate the encoder
 *    slowly.  Note that some changes are "lost", due to the
 *    delay in the task.  This shows a difficulty with polling
 *    for updates in loop(), when also doing other work in loop().
  * 5. In function DoFakeTask(), increase the delay to
 *    1000 milliseconds, and re-upload the sketch.
 *    What will happen when you rotate the encoder?
*
 * When done, move to the next example.
 */

#include "Arduino.h"
#include "qdec.h"

const int ROTARY_PIN_A = 3; // the first pin connected to the rotary encoder
const int ROTARY_PIN_B = 2; // the second pin connected to the rotary encoder

::SimpleHacks::QDecoder qdec(ROTARY_PIN_A, ROTARY_PIN_B, true); // the library class...

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
  ::SimpleHacks::QDECODER_EVENT event = qdec.update();

  // increment for clockwise, decrement for counter-clockwise
  if (event & ::SimpleHacks::QDECODER_EVENT_CW) {
    rotaryCount = rotaryCount + 1;
    Serial.print("change: "); Serial.println(rotaryCount);
  } else if (event & ::SimpleHacks::QDECODER_EVENT_CCW) {
    rotaryCount = rotaryCount - 1;
    Serial.print("change: "); Serial.println(rotaryCount);
  }
} // end of void loop()
