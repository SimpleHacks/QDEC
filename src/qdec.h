/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 by SimpleHacks, Henry Gabryjelski
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef __INC_SIMPLEHACKS_QDEC
#define __INC_SIMPLEHACKS_QDEC

#include <Arduino.h>
#include <stdint.h>

#if defined(SIMPLEHACKS_QDECODER_NEVER_INLINE)
    // yes, the __noinline__ attribute still uses inline, so that even non-template
    // functions can be defined in the header file.
    #define SIMPLEHACKS_INLINE_ATTRIBUTE __attribute__((noinline)) inline
#else     
    #define SIMPLEHACKS_INLINE_ATTRIBUTE __attribute__((always_inline)) inline
#endif    

// This library uses a namespace (e.g., SimpleHacks::QDecoder instead of just QDecoder).
// However, it's very easy to make this effectively disappear!
// As an example, at the start of a function (or right after all headers included)
// simply add the following lines:
//     using SimpleHacks::QDECODER_EVENT;
//     using SimpleHacks::QDecoder;
//     using SimpleHacks::QDECODER_EVENT;
// Or, if you want all the types to be avabile, you could instead add the line:
//     using namespace SimpleHacks;
//
// See the examples for more options...

namespace SimpleHacks {

    typedef enum : uint8_t {
        QDECODER_EVENT_NONE   = 0x00u,
        QDECODER_EVENT_CW     = 0x80u,
        QDECODER_EVENT_CCW    = 0x40u,
    } QDECODER_EVENT;

    /*
    // use of this namespace helps hide all the gory implementation details
    // from auto-completion and similar editor niceties.
    // it also provides a signal to the user that these elements may change
    // between releases, whether in name, underlying values, existence, or behavior.
    */
    namespace Internal {
        // state is internal... needs never be exposed to callers
        typedef enum : uint8_t {
            QDECODER_STATE_START      = 0x00u,
            QDECODER_STATE_CW_A       = 0x01u,
            QDECODER_STATE_CW_B       = 0x02u,
            QDECODER_STATE_CW_C       = 0x03u,
            QDECODER_STATE_MID        = 0x04u,
            QDECODER_STATE_CCW_A      = 0x05u,
            QDECODER_STATE_CCW_B      = 0x06u, 
            QDECODER_STATE_CCW_C      = 0x07u,
        } QDECODER_STATE;
        
        static const uint8_t QDECODER_EVENT_BITMASK    = 0xC0; // events are the most significant two bits
        static const uint8_t QDECODER_STATE_BITMASK    = 0x07; // states are in the least significant three bits
        static const uint8_t QDECODER_RESERVED_BITMAST = 0x31; // four bits still available for use in later versions of the library

        // Tables of state transitions + events.
        // See ./docs/full_step.svg for full-step state diagram.
        // See ./docs/half_step.svg for half-step state diagram.
        // Note that the unused states exist in each table, with all entries pointing to START
        // This allows the QDECODER_STATE enumeration to be common between the two tables.
        // (The cost is an extra four / eight data bytes (flash) for full-step / half-step tables, respectively.)
        static const uint8_t FULL_STEP_STATE_TRANSITIONS[8][4] = {
            // 0 = start
            {QDECODER_STATE_START, QDECODER_STATE_CW_A,  QDECODER_STATE_CCW_A, QDECODER_STATE_START },
            // 1 = CW_A
            {QDECODER_STATE_CW_B,  QDECODER_STATE_CW_A,  QDECODER_STATE_START, QDECODER_STATE_START },
            // 2 = CW_B
            {QDECODER_STATE_CW_B,  QDECODER_STATE_CW_A,  QDECODER_STATE_CW_C,  QDECODER_STATE_START },
            // 3 = CW_C
            {QDECODER_STATE_CW_B,  QDECODER_STATE_START, QDECODER_STATE_CW_C,  QDECODER_STATE_START | QDECODER_EVENT_CW },
            // 4 = mid (unused in full step mode ...)
            {QDECODER_STATE_START, QDECODER_STATE_START, QDECODER_STATE_START, QDECODER_STATE_START },
            // 5 = CCW_A
            {QDECODER_STATE_CCW_B, QDECODER_STATE_START, QDECODER_STATE_CCW_A, QDECODER_STATE_START },
            // 6 = CCW_B
            {QDECODER_STATE_CCW_B, QDECODER_STATE_CCW_C, QDECODER_STATE_CCW_A, QDECODER_STATE_START },
            // 7 = CCW_C
            {QDECODER_STATE_CCW_B, QDECODER_STATE_CCW_C, QDECODER_STATE_START, QDECODER_STATE_START | QDECODER_EVENT_CCW },
        };
        static const uint8_t HALF_STEP_STATE_TRANSITIONS[8][4] = {
            // 0 = start
            {QDECODER_STATE_MID,   QDECODER_STATE_CW_A,  QDECODER_STATE_CCW_A, QDECODER_STATE_START },
            // 1 = CW_A
            {QDECODER_EVENT_CW |
             QDECODER_STATE_MID,   QDECODER_STATE_CW_A,  QDECODER_STATE_START, QDECODER_STATE_START },
            // 2 = CW_B (unused in half-step mode)
            {QDECODER_STATE_START, QDECODER_STATE_START, QDECODER_STATE_START, QDECODER_STATE_START },
            // 3 = CW_C
            {QDECODER_STATE_MID,   QDECODER_STATE_MID,   QDECODER_STATE_CW_C,  QDECODER_STATE_START | QDECODER_EVENT_CW },
            // 4 = mid
            {QDECODER_STATE_MID,   QDECODER_STATE_CCW_C, QDECODER_STATE_CW_C,  QDECODER_STATE_START },
            // 5 = CCW_A
            {QDECODER_EVENT_CCW |
            QDECODER_STATE_MID,   QDECODER_STATE_START, QDECODER_STATE_CCW_A, QDECODER_STATE_START },
            // 6 = CCW_B (unused in half-step mode)
            {QDECODER_STATE_START, QDECODER_STATE_START, QDECODER_STATE_START, QDECODER_STATE_START },
            // 7 = CCW_C
            {QDECODER_STATE_MID,   QDECODER_STATE_CCW_C, QDECODER_STATE_MID,   QDECODER_STATE_START | QDECODER_EVENT_CCW},
        };
    }

    class QDecoder {
    public: // special members
        ~QDecoder() =default;
        QDecoder() =delete;
        QDecoder(const QDecoder&) =delete;
        QDecoder& operator=(const QDecoder&) =delete;
        // move constructor -- not declared
        // QDecoder(QDecoder&&);
        // move assignment operator -- not declared
        // QDecoder& operator=(QDecoder&&);
    public: // actual constructors
        QDecoder(uint16_t pinA, uint16_t pinB);
        QDecoder(uint16_t pinA, uint16_t pinB, boolean useFullStep);
    public: // User API
        // begin() sets the pins to INPUT_PULLUP
        SIMPLEHACKS_INLINE_ATTRIBUTE void begin() {
            pinMode(_pinA, INPUT_PULLUP);
            digitalWrite(_pinA, HIGH); // turn on pullup resistor
            pinMode(_pinB, INPUT_PULLUP);
            digitalWrite(_pinB, HIGH); // turn on pullup resistor
        };

        // update() reads the pins, changes state appropriately, and returns a
        // clockwise or counter-clockise (aka anti-clockwise) event, as any
        // detected state transition requires.
        // This function is designed to be callable either from an ISR or timer,
        // and thus focuses on efficiency.  See https://github.com/SimpleHacks/QDEC/
        // for explanation and state diagrams.
        SIMPLEHACKS_INLINE_ATTRIBUTE QDECODER_EVENT update() {
            // newPinState is defined so that it maps to the INDEX
            // to use in the state transition table.
            // The below three lines optimize well on modern compilers.
            uint8_t newPinState = 0;
            if (digitalRead(_pinA)) { newPinState |= (1 << 1); }
            if (digitalRead(_pinB)) { newPinState |= (1 << 0); }

            // Use the state transition table to determine the next state and any events to generate
            uint8_t nextStateWithEvents =
                _useFullStep ?
                    ( SimpleHacks::Internal::FULL_STEP_STATE_TRANSITIONS[ _CurrentState ][ newPinState ] ) :
                    ( SimpleHacks::Internal::HALF_STEP_STATE_TRANSITIONS[ _CurrentState ][ newPinState ] ) ;
            
            // Save the new current state, and return the event portion
            _CurrentState = nextStateWithEvents & SimpleHacks::Internal::QDECODER_STATE_BITMASK;
            return static_cast<QDECODER_EVENT>( nextStateWithEvents & SimpleHacks::Internal::QDECODER_EVENT_BITMASK );
         };
    
    private:
        const uint16_t _pinA;
        const uint16_t _pinB;
        const boolean  _useFullStep;
              uint8_t  _CurrentState;
    };

    SIMPLEHACKS_INLINE_ATTRIBUTE QDecoder::QDecoder(uint16_t pinA, uint16_t pinB)
        : _pinA(pinA), _pinB(pinB), _useFullStep(false), _CurrentState(Internal::QDECODER_STATE_START) {};

    SIMPLEHACKS_INLINE_ATTRIBUTE QDecoder::QDecoder(uint16_t pinA, uint16_t pinB, boolean useFullStep)
        : _pinA(pinA), _pinB(pinB), _useFullStep(useFullStep), _CurrentState(Internal::QDECODER_STATE_START) {};

    // The templated class is defined here.
    //
    // Some potential benefits to a template version:
    // * Many optimizations, especially important on low-power boards:
    //   + inline-optimizations to avoid function calls
    //   + only static functions, so zero run-time memory allocation
    //   + code conditioned on template arguments is optimized away if
    //     unused, allowing the code to handle the various options,
    //     while the resulting binary only contains used code paths
    // * The optimizations above also result in FAST code, critical
    //   for ISR-driven QDECs....
    //
    // Using a template does have some downsides:
    // * If you have many rotary encoders / quadrature encoders,
    //   each QDEC instance will generate a distinct class, with its own
    //   instructions for each function, because the template's pins will
    //   be different for each instance.  Depending on optimizations
    //   applied, this *might* result in a larger code size versus
    //   an implementation with a constructor-based class that stores
    //   the template parameters in member variables.
    // * Templates can only be used when the template parameters (e.g., pins)
    //   can be determined at compile-time.  This is the reason the
    //   class-based implementation is provided.
    //
    template <uint32_t _ARDUINO_PIN_A, uint32_t _ARDUINO_PIN_B, uint32_t _USE_FULL_STEP = 0>
    class QDec {

    public: // special members
        // all six special members are defined as deleted,
        // as the templated version only uses static functions,
        // and thus should never be instantiated.

        // destructor
        ~QDec() =delete;
        // parameterless (default) constructor
        QDec() =delete;
        // copy constructor
        QDec(const QDec&) =delete;
        // copy assignment operator
        QDec& operator=(const QDec&) =delete;
        // move constructor
        QDec(QDec&&) =delete;
        // move assignment operator
        QDec& operator=(QDec&&) =delete;

    public: // public API
        // Initialization -- sets pins to correct input state
        SIMPLEHACKS_INLINE_ATTRIBUTE static void begin() {
            pinMode(_ARDUINO_PIN_A, INPUT);
            digitalWrite(_ARDUINO_PIN_A, HIGH); // turn on pullup resistor
            pinMode(_ARDUINO_PIN_B, INPUT_PULLUP);
            digitalWrite(_ARDUINO_PIN_B, HIGH); // turn on pullup resistor
        }

        // update() reads the pins, changes state appropriately, and returns a
        // clockwise or counter-clockise (aka anti-clockwise) event, as any
        // detected state transition requires.
        // This function is designed to be callable either from an ISR or timer,
        // and thus focuses on efficiency.  See https://github.com/SimpleHacks/QDEC/
        // for explanation and state diagrams.
        SIMPLEHACKS_INLINE_ATTRIBUTE static SimpleHacks::QDECODER_EVENT update() {
            // On ARM using gcc 8.2, compiles to fewer than 100 bytes
            // (< 25 instructions), with inlining disabled.
            // See https://godbolt.org/z/agVxA-
            static uint8_t s_CurrentState = Internal::QDECODER_STATE_START;

            // newPinState is defined so that it maps to the INDEX
            // to use in the state transition table.
            // The below three lines optimize well on modern compilers.
            uint8_t newPinState = 0;
            if (digitalRead(_ARDUINO_PIN_A)) { newPinState |= (1 << 1); }
            if (digitalRead(_ARDUINO_PIN_B)) { newPinState |= (1 << 0); }

            // Use the state transition table to determine the next state and any events to generate
            Internal::QDECODER_STATE nextStateWithEvents = static_cast<Internal::QDECODER_STATE>(
                _USE_FULL_STEP ?
                    ( Internal::FULL_STEP_STATE_TRANSITIONS[ s_CurrentState ][ newPinState ] ) :
                    ( Internal::HALF_STEP_STATE_TRANSITIONS[ s_CurrentState ][ newPinState ] ) 
                );
            
            // Save the new current state, and return the event portion
            s_CurrentState = nextStateWithEvents & Internal::QDECODER_STATE_BITMASK;
            return static_cast<QDECODER_EVENT>( nextStateWithEvents & Internal::QDECODER_EVENT_BITMASK );
        }
    };
}


#endif // #ifndef __INC_SIMPLEHACKS_QDEC



