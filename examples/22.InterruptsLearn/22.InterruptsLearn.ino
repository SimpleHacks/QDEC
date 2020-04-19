/* SKETCH SUMMARY
  This uses SimpleHacks' QDEC library (aka Quadrature Decoder, aka rotary decoder) 
  to obtain clockwise / counterclockwise events.
  
  This sketch uses the QDecoder class.  Rather than spinning in a tight loop,
  or attempting to guess how often to poll the device, this example uses a
  feature of the processor to detect when a change in the pin's signal occurs,
  and then "interrupt" what's executing to run a function that's "attached" or
  "handles" that interrupt, also known as an Interrupt Service Routine (ISR).
  (When the ISR is finished, the interrupted function resumes execution.)

  The setup() function for this sketch attaches the same ISR for both pins.
  An ISR must not return a value, and takes no arguments.  Here, the function
  is named `IsrForQDEC()`, and is configured to be called whenever the pins
  change (e.g., both from 0 to 1, and from 1 to 0).  The ISR calls qdec.update()
  to update the decoder state, which returns if an event occurred.  When an
  event occurred, the ISR increments (or decrements) a global variable.

  The QDecoder object is connected to pin 2 and pin 3, as these are the only pins
  that support "interrupt on change" on the Arduino Nano.  
  If using a different board, check the board description for which pins can use
  "interrupt on change".  For Arduino boards, the list of pins can be found at:
  https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/

  Using interrupts is an advanced topic.  Two things of note:
  1. The `rotaryCount` variable is marked `volatile`.  Any variable that is
     *both* modified by an ISR and used outside the ISR must be so marked.
  2. The `loop()` function starts by copying the `volatile` variable `rotaryCount`
     to a local variable (`newCount`).  This allows the ISR to continue to change
     `rotaryCount`, without affecting logic in `loop()` that relies on `newCount`.
     Any later changes caused by an ISR are picked up in the next iteration of `loop()`.
  
  The circuit requires the following:
  * an arduino with two digital input pins (here, using D2 and D3)
  * a rotary encoder
    * pin A and pin B connect to Arduino pins D2 and D3
    * the third pin (sometimes designated pin C) connects to ground (GND)
  See circuit diagram at: https://github.com/SimpleHacks/QDEC/docs/

  Created 2019-09-29
  By Henry Gabryjelski
*/
/* Learn by doing:
 * Best results when using a rotary encoder with detents (clicks).
 * 1. Upload the sketch, and enable the serial monitor
 *    (Tools -> Serial Monitor)
 * 2. Rotate the encoder and see how much the value changes per
 *    detent (click).  For example, it's likely going to change
 *    either 1 or 2 values per click.  Note which direction
 *    (clockwise or anti-clockwise) increases the value, and
 *    which decreases the value.
 * 3. Swap which pins are defined as ROTARY_PIN_A and
 *    ROTARY_PIN_B, re-upload the sketch. What is different
 *    when you rotate the decoder now?
 *    (Revert this change as desired.)
 * 4. In function DoFakeTask(), increase the delay to
 *    25 milliseconds, and re-upload the sketch.
 *    Rotate the encoder quickly, and then rotate the encoder 
 *    slowly.  Note that none of the rotations are "lost",
 *    even when the value changes by more than one per loop.
 *    How is this different than the LoopLearn example?
 * 5. In function DoFakeTask(), increase the delay to
 *    1000 milliseconds, and re-upload the sketch.
 *    What will happen when you rotate the encoder?
 */

#include "Arduino.h"
#include "qdec.h"

const int ROTARY_PIN_A = 3; // the first pin connected to the rotary encoder
const int ROTARY_PIN_B = 2; // the second pin connected to the rotary encoder

::SimpleHacks::QDecoder qdec(ROTARY_PIN_A, ROTARY_PIN_B, true); // the library class...

// Stores a relative value based on the clockwise / counterclockwise events
volatile int rotaryCount = 0;
// Used in loop() function to track the value of rotaryCount from the
// prior time that loop() executed (to detect changes in the value)
int lastLoopDisplayedRotaryCount = 0;

// This is the ISR (interrupt service routine).
// This ISR fires when the pins connected to the rotary encoder change
// state.  Therefore, the update() function is called to update the
// decoder state and then (if an event was indicated), the rotaryCount
// variable is updated.
// CAUTION: some Arduion functions will not work if called from an ISR.
void IsrForQDEC(void) { // do absolute minimum possible in any ISR ...
  // placing the `using namespace` line into a function limits
  // allows accessing the types and functions of that namespace,
  // but only while in this function.
  using namespace ::SimpleHacks;
  QDECODER_EVENT event = qdec.update();
  if (event & QDECODER_EVENT_CW) {
    rotaryCount = rotaryCount + 1;
  } else if (event & QDECODER_EVENT_CCW) {
    rotaryCount = rotaryCount - 1;
  }
  return;
}

// The only change to the setup() routine is the two lines
// that attach the interrupt for the two pins to the ISR.
void setup() {
  delay(2000);
  
  Serial.begin(115200);
  while (!Serial) {} // wait for serial port to connect... needed for boards with native USB serial support
  Serial.print("Beginning QDecoder Sketch ");  
  Serial.println(__FILE__); // becomes the sketch's filename during compilation

  // initialize the rotary encoder
  qdec.begin();

  // attach an interrupts to each pin, firing on any change in the pin state
  // no more polling for state required!
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A), IsrForQDEC, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_B), IsrForQDEC, CHANGE);
}

void DoFakeTask(void) {
  // Simulate work that takes significant processing time
  unsigned long millisecondsToDelay = 0;
  //millisecondsToDelay = 25;
  //millisecondsToDelay = 1000;
  delay(millisecondsToDelay);
}

void loop() {
  DoFakeTask();

  // Because we are not getting the events from the library directly,
  // we have to detect the change in value of the rotaryCount variable.
  // Because an interrupt could change the value of rotaryCount at any
  // time, read its value into a local variable, which won't be affected
  // even if another ISR fires and changes rotaryCount.
  int newValue = rotaryCount;
  if (newValue != lastLoopDisplayedRotaryCount) {
    // Also get the difference since the last loop() execution
    int difference = newValue - lastLoopDisplayedRotaryCount;
    
    // act on the change: e.g., modify PWM to be faster/slower, etc.
    lastLoopDisplayedRotaryCount = newValue;
    Serial.print("Change: "); Serial.print(newValue);
    Serial.print(" ("); Serial.print(difference); Serial.println(")");
  }

} // end of void loop()
