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
static const struct pio_program qdecpio_program = {
    .instructions = qdecpio_program_instructions,
    .length = 12, // For C++ ... would use SAFER_ARRAY_SIZE(qdecpio_program_instructions)
    .origin = -1,
};
static inline pio_sm_config qdecpio_program_get_default_config(uint offset) {
    pio_sm_config config = pio_get_default_sm_config();
    sm_config_set_wrap(&config, offset + qdecpio_wrap_target, offset + qdecpio_wrap);
    return config; // returns a copy of the structure ... usually optimized well inline
}
static inline void qdecpio_program_init(PIO pio, int smIdx, uint offset, uint pin_a, uint pin_b) {
    pio_sm_config config = qdecpio_program_get_default_config(offset);
    sm_config_set_in_pins(&config, pin_a); // sm will wait on edges on this pin's signal
    sm_config_set_jmp_pin(&config, pin_b); // sm will conditionally jump based on this pin's signal
    static const bool shift_direction_right = false;
    static const bool enable_autopush = true;
    static const uint8_t autopush_threshold = 32;
    sm_config_set_in_shift(&config, shift_direction_right, enable_autopush, autopush_threshold);

    pio_sm_init(pio, smIdx, offset, &config);
    pio_sm_set_enabled(pio, smIdx, true);
}

typedef struct allocated_pio_sm_program { // Valid only if Pio is valid pointer (!= nullptr)
    PIO Pio;                // will be nullptr on failure
    uint ProgramOffset;
    int StateMachineIdx;
};
static const allocated_pio_sm_program invalid_pio_sm_program_allocation = {
    .Pio = nullptr,
    .ProgramOffset = (uint)-1,
    .StateMachineIdx = -1,
};
static inline allocated_pio_sm_program qdecpio_try_to_allocate_specific_pio(PIO pio) {
    if (!pio_can_add_program(pio, &qdecpio_program)) {
        return invalid_pio_sm_program_allocation;
    }
    int smIdx = pio_claim_unused_sm(pio, false); // false == return -1 on failure (not panic)
    if (smIdx == -1) {
        return invalid_pio_sm_program_allocation;
    }
    // No more failures paths from here onwards
    uint offset = pio_add_program(pio, &qdecpio_program); // panics on failure
    allocated_pio_sm_program x = {
        .Pio = pio,
        .ProgramOffset = offset,
        .StateMachineIdx = smIdx,
    };
    return x;
}
// user code can call this to initialize on any available PIO / state machine
static inline allocated_pio_sm_program qdecpio_try_to_initialize(uint pin_a, uint pin_b) {
    allocated_pio_sm_program x = qdecpio_try_to_allocate_specific_pio(pio0);
    if (x.pio == nullptr) {
        x = qdecpio_try_to_allocate_specific_pio(pio1);
    }
    if (x.pio != nullptr) {
        qdecpio_program_init(x.Pio, x.StateMachineIdx, x.ProgramOffset, pin_a, pin_b);
    }
    return x;
}
