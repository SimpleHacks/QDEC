# PIO Quadrature Decoder

The stretch goal of this investigation was to have
a single-state-machine PIO program, which would handle
all the decoding of the quadrature signal, and keep
a count (decreasing for one direction, increasing the
other direction).

The end result is an interesting hack.  This documents
the learning journey.  I started with an expert level of
understanding of C, C++, and embedded systems.  I also
started with an intermediate level of understanding
PIO assembly.

## Not as simple ...

This turned out more difficult than I initially thought,
at least to make it 100% reliable and efficient.

Some of the relevant limitations of PIO and its instructions set:

* `WAIT` opcode cannot wait on pin **change** ... only a specific polarity
    * thus, have to double the state machine instructions,
      based on the prior state of Signal A
* No instruction allows to **read** (non-blocking) an IRQ state
    * thus, cannot use IRQ state to pass data betwen state machines
    * thus, cannot have MCU set an IRQ to request data
    * PIO SM could stall forever waiting on the IRQ
* Can only wait on, set, or clear a **single** IRQ
    * thus, cannot wait on multiple IRQ at once
* Cannot directly connect two state machine FIFO queues together
    * thus, cannot cheat by having one state machine feed
      data directly into a second state machine
    * rather, one could connecting a DMA channel to read from
      one FIFO and push to the other, but then the overhead increases
      to two PIO **AND** a DMA channel ... which is not preferred.
* Currently no explicit decrement instruction
    * `JMP (X--) LABEL_FOR_NEXT_INSTRUCTION` is, essentially, `DEC x`
    * To realize this, folks have to stumble upon, and understand,
      an unrelated instruction (sigh)
* Cannot **atomically** increment a register
    * Increment takes a minimum of three cycles
    * During two of those cycles, the register value is **_neither_**
      the pre-increment nor post-increment value
    * Increment looks like the following:
      ```asm
      ; Twos-complement encoding identity: X + Y === ~((~X) - Y)
      ; X + 1 === ~((~X)--)
          MOV X, ~X                           ; value neither pre- nor post-increment
          JMP (X--) Label_IrrelevantDecrement ; value neither pre- nor post-increment
      Label_IrrelevantDecrement:
          MOV X, ~X                           ; value stable at post-increment
      ```
* `JMP PIN <LABEL>` can only use a **single, preconfigured** GPIO
* No `XOR` instruction
    * conditional jump on !X (when x == 0)
    * conditional jump on !Y (when y == 0)
    * conditional jump (with decrement side-effect) of X (when x != 0)
    * conditional jump (with decrement side-effect) of Y (when y != 0)
    * MOV allows bitwise negation
    * MOV allows bit-reversal
    * SET can create values 0..31
    * Thus ... non-trivial to XOR
* If the program uses `MOV EXEC` or `OUT EXEC`, then the CPU
  cannot write to the `SMx_INSTR` register, because they use
  the same hardware resource....

Probably the most important of the above limitations is that,
while there is an atomic decrement, there is **_no_** atomic
increment.  Using the three-instruction increment makes it
impossible for the CPU to reliably retrieve **_only_** the
pre-increment value **_or_** the post-increment value of that register
through writing to `SMx_INSTR`.

In combination, these limitations prevent some of the more
obvious design choices.  Time to review some historical
(early) quadrature decoder circuits.  Maybe they can guide
towards a solution?

## Brief background on quadrature decoders

This is intentionally not a full description.  Look elsewhere
for descriptions of optical sensors and offset cutouts, etc.

### Focus on the signals

There are TWO input signals, and they are laid out so that,
for a given maximum speed:

* (digitally) edges are always offset by a minimum time
* (analog) edges have a maximum signal transition time
* (physical) edges bounce, adding to maximum signal transition time

Because the edges have a minimum time delta, so long as the
encoder is rated for the speed in which it's used, the
non-transitioning signal is guaranteed to have a stable value.
(e.g., guaranteed avoidance of metastable input)

In examples and diagrams (including here), the speed is
usually constant as the diagrams are simpler.  Visually,
the two signals are 50% duty waveforms, offset by 90°.

The useful part of sampling the other signal on the first
signal's edge:  Whether the edge was rising or falling, in
combination with the second signal's value, indicates not
only movement, but a **_direction_** of that movement.
 
## Visuals (amazing ASCII art!)

Here are two diagrams, showing an example of the signals
when moving in each direction.  Note how, at each **_rising_**
edge for Signal A, Signal B is high for clockwise movement,
and low for anti-clockwise movement.  Similarly, at each
**_falling_** edge of Signal A, Signal B is low for
clockwise movement, but high for anti-clockwise movement.

### Figure 1: clockwise direction (increment)

```
               _____       _____       _____
Signal A _____|     |_____|     |_____|     |__
              ^     :     ^     :     ^     :
              |     |     |     |     |     |
              :     v     :     v     :     v
            _____       _____       _____
Signal B __|     |_____|     |_____|     |_____
```

### Figure 2: anti-clockwise direction (decrement)

```
               _____       _____       _____
Signal A _____|     |_____|     |_____|     |__
              ^     :     ^     :     ^     :
              |     |     |     |     |     |
              :     v     :     v     :     v
         __       _____       _____       _____
Signal B   |_____|     |_____|     |_____|     
```

### Table 1: Edge A + Signal B --> Direction

Edge A   | Signal B | Direction
---------|----------|----------------
Rising   | LOW      | anti-clockwise
Rising   | HIGH     | clockwise
Falling  | LOW      | clockwise
Falling  | HIGH     | anti-clockwise

### Table 2: Edge B + Signal A --> Direction

Could also (and simultaneously) swap the pins:

Edge B   | Signal A | Direction
---------|----------|----------------
Rising   | LOW      | clockwise
Rising   | HIGH     | anti-clockwise
Falling  | LOW      | anti-clockwise
Falling  | HIGH     | clockwise
 
## Failed solutions

<details><summary>click to expand...</summary><P/>

### How does the CPU get the value of X?

This is trickier than it might (at first) seem.
Because the increment operation is NOT atomic
(it takes multiple state machine cycles).

If the X register operations were always atomic,
AND the program doesn't use `MOV EXEC` AND the
program doesn't use `OUT EXEC`, AND autopull
is enabled... THEN the CPU can write `IN X, 32`
to the `SMx_INSTR` register, followed by
reading the result from the RX FIFO.

However, because incrementing the X register
requires (minimum) 3 operations, there are two
state machine cycles where the value in the X
register is invalid (storing neither the
pre-increment nor post-increment value).

Unfortunately, those invalid value cannot always
be distinguished from valid values.

### Solution #1 ... half-baked

With a careful review of the instructions listed
above, I noticed another way to allow the CPU to
interrupt the state machine's execution (at any
time), and still get a valid value into the FIFO.

None of the quadrature decoding / tracking uses
the FIFO. PIO has a feature called "autopull",
which is normally quite useful. But, by disabling
autopull, the state machine is allowed to write
(and overwrite) the ISR value infinitely, without
ever blocking due to a full FIFO.

Thus, if the state machine, after incrementing
or decrementing the X register, shifts the new
value into the ISR:
```asm
    in X, 32 ; controlled by state machine, so always valid
```

Then instead of `IN X, 32`, the CPU could write
`PUSH noblock` to the `SMx_INSTR` register, resulting
in the RX FIFO getting the value from the `ISR`
register.

Looks good, right?
 
### Why is it only half-baked?

The `PUSH noblock` instruction clears the value
stored in the `ISR` register.  This means that
the state machine would need to update the `ISR`
again, before the CPU could attempt to read
the value again.

However, the state machine doesn't have an easy
way to indicate to the CPU that it's created a
new value ... at least, not a way which the
CPU can clear in a manner that allows this
without adding more resources (e.g., DMA channel).

Why?  Most of the time, the SM will be stalled,
waiting for an edge.  It cannot respond to
CPU actions, nor update any state, while it's
stalled.  The SM also cannot read (without
also waiting for) and IRQ.

Thus, the next time the CPU force-executes
`PUSH noblock`, the value written to the FIFO
will be zero, unless the encoder was turned
in the interrim.

Sigh... there must be a way...

The state machine can't remove or replace
a value sitting in the RX FIFO (the pipe
to the CPU).  Thus, it cannot place data
there unless the CPU will read it ...
else the FIFO will fill with (stale) values
and will cause the SM to stall when the
FIFO fills.

</details><hr/>

## Starting fresh (again)

The root problem is that, because X sometimes
had invalid values, the CPU could not arbitrarily
interrupt the state machine to copy the register
value to the RX FIFO.

The reason it sometimes had invalid values was
because the register increment was non-atomic.
    
The solution: use the `X` register to count
steps in one direction, and use the `Y` register
to count steps in the other direction.  Storing
the negated value is just as valid as the positive
value, so rather than increment to count, decrement.

As a result, all operations on the `X` and `Y`
registers are atomic.  On the CPU side, rather
than writing a single `IN X, 32` to `SMx_INSTR`,
it writes two instructions:
```asm
    IN Y, 32
    IN X, 32
```

This pushes two 32-bit values into the FIFO.
The CPU then reads the values, expressing the
following pseudocode:
```
    int32_t regY = read_rx_fifo(...);
    int32_t regX = read_rx_fifo(...);
    int32_t result = Y - X;
    return result;
```

## Non-validated PIO assembly

```asm
.program qdec_pio

init:                          ; SM_RESTART does not clear X or Y
   set X, 0                    ; 0 : set initial value for X
   set Y, 0                    ; 1 : set initial value for Y
    
.wrap_target:                  ; wrap to here ... save one JMP instruction

p1__wait_falling:              ; label = 2
    wait 0 pin 0               ;  2 : wait for falling edge
    jmp pin, p1__falling_high  ;  3 : branch based on other signal
p1__falling_low:               ;  label = 4
    jmp (x--) p1__wait_rising  ;  4 : X-- / clockwise
    jmp p1__wait_rising        ;  5 : edge case: pre-decrement, x was zero
p1__falling_high:              ;  label = 6
    jmp (y--) p1__wait_rising  ;  6 : Y-- / anti-clockwise

p1__wait_rising:               ;  label = 7
    wait 1 pin 0               ;  7 : wait for rising edge
    jmp pin, p1__rising_high   ;  8 : branch based on other signal
p1__rising_low:                ;  label = 9
    jmp (y--) p1__wait_falling ;  9 : Y-- / anti-clockwise
    jmp p1_wait_falling        ; 10 : edge case: pre-decrement, y was zero
p1__rising_high:               ;  label = 11
    jmp (x--) p1__wait_falling ; 11 : X-- / clockwise
.wrap
```

## Same code as 16-bit machine code (pre-deployment ... address offset == 0)

```C++
#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

static const uint8_t  qdecpio_wrap_target = 2;
static const uint8_t  qdecpio_wrap = 11; // when this instruction executes, it auto-wraps
static const uint16_t qdecpio_program_instructions[] = {
    0xE020, // 0b_111_00000_001_00000,  //  0: set X, 0
    0xE040, // 0b_111_00000_010_00000,  //  1: set Y, 0
    0x2020, // 0b_001_00000_0_01_00000, //  2: wait 0, pin 0
    0x00C6, // 0b_000_00000_110_00110,  //  3: jmp pin, 6
    0x0047, // 0b_000_00000_010_00111,  //  4: jmp (x--), 7
    0x0007, // 0b_000_00000_000_00111,  //  5: jmp 7
    0x0087, // 0b_000_00000_100_00111,  //  6: jmp (y--), 7
    0x20A0, // 0b_001_00000_1_01_00000, //  7: wait 1, pin 0
    0x00CB, // 0b_000_00000_110_01011,  //  8: jmp pin, 11
    0x0082, // 0b_000_00000_100_00010,  //  9: jmp (y--), 2
    0x0002, // 0b_000_00000_000_00010,  // 10: jmp 2
    0x0042, // 0b_000_00000_010_00010,  // 11: jmp (x--), 2
};
```
