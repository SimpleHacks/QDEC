/*
  06.DynamicPinConfig

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
/* Difference from 04.LoopNamespace
 *  
 * parameterless qdec constructor used (pins not specified)
 * 
 *
 * Move to the next example.
 */

#include "Arduino.h"
#include "qdec.h"

using namespace ::SimpleHacks;

const int ROTARY_PIN_A = 3; // the first pin connected to the rotary encoder
const int ROTARY_PIN_B = 2; // the second pin connected to the rotary encoder

QDecoder qdec; // the library class...

// Stores a relative value based on the clockwise / counterclockwise events
int rotaryCount = 0;
unsigned int  lastConfig = 0;
unsigned long lastConfigured;

void UpdateConfig() {
  // the `%` operator is the remainder after division.
  // for unsigned values, this means the result is in the range 0 .. (divisor-1).
  // we use this to select which of four configurations to put the encoder into
  unsigned int config = lastConfig % 4;
  lastConfig++;

  int pinA;
  int pinB;
  bool fullStep;

  // what options to use this time around?
  switch (config) {
    case 0:
      // start at same config as prior sketch
      pinA = ROTARY_PIN_A;
      pinB = ROTARY_PIN_B;
      fullStep = true;
      break;
    case 1:
      // start at same config as prior sketch
      pinA = ROTARY_PIN_A;
      pinB = ROTARY_PIN_B;
      fullStep = false;
      break;
    case 2:
      // start at same config as prior sketch
      pinA = ROTARY_PIN_B;
      pinB = ROTARY_PIN_A;
      fullStep = true;
      break;
    case 3:
      // start at same config as prior sketch
      pinA = ROTARY_PIN_B;
      pinB = ROTARY_PIN_A;
      fullStep = false;
      break;
  }

  Serial.print("New configuration mode "); Serial.print(config);
  Serial.print(": PinA = "); Serial.print(pinA);
  Serial.print(", PinB = "); Serial.print(pinB);
  Serial.print(", fullStep = "); Serial.println(fullStep);
  qdec.setPinA(pinA);
  qdec.setPinB(pinB);
  qdec.setFullStep(fullStep);
}


void setup() {
  delay(2000);
  
  Serial.begin(115200);
  while (!Serial) {} // wait for serial port to connect... needed for boards with native USB serial support
  Serial.print("Beginning QDecoder Sketch ");  
  Serial.println(__FILE__); // becomes the sketch's filename during compilation

  // initialize the rotary encoder
  UpdateConfig();
  qdec.begin();
  lastConfigured = millis();
}



void HandleFakeConfigurationChange(void) {
  // Run in each configuration for ten seconds, 
  // disabling the encoder for two seconds between each config.
  bool wasStarted = qdec.getIsStarted();
  unsigned long now = millis();
  unsigned long timePassed = now - lastConfigured;
  if (wasStarted && (timePassed > 10000)) {
    lastConfigured = now;

    qdec.end();
    Serial.println("Emulating input being disabled"); // could also occur transitioning to low-power
    qdec.setPinA(QDecoder::QDECODER_INVALID_PIN); // only allowed when not started
    qdec.setPinB(QDecoder::QDECODER_INVALID_PIN); // only allowed when not started

  } else if ((!wasStarted) && (timePassed > 2000)) {
    lastConfigured = now;

    Serial.println("Emulating new configuration and input being re-enabled"); // or when waking from low-power
    UpdateConfig();
    qdec.begin();

  }
}

void loop() {
  HandleFakeConfigurationChange();

  // poll the state of the quadrature encoder,
  // and storing the result (which is an event, if one occurred this time)
  // NOTE: if the qdec is not started, this does nothing.
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
