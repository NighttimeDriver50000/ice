#include <stdint.h>
#include "irobled.h"
#include "cmod.h"
#include "oi.h"

// The current state of the leds.
struct {
    uint8_t bits;
    uint8_t color;
    uint8_t intensity;
} iroblibState;

void irobledCmd(uint8_t bits, uint8_t color, uint8_t intensity) {
    // Modify the state
    iroblibState.bits = bits;
    iroblibState.color = color;
    iroblibState.intensity = intensity;
    // Update
    irobledUpdate();
}

void irobledUpdate(void) {
    // Send the led command using the current state
    byteTx(CmdLeds);
    byteTx(iroblibState.bits);
    byteTx(iroblibState.color);
    byteTx(iroblibState.intensity); 
}

void irobledInit(void) {
    irobledCmd(NEITHER_ROBOT_LED, POWER_LED_ORANGE, 0xFF);
}

void powerLedSet(uint8_t color, uint8_t intensity) {
    irobledCmd(iroblibState.bits, color, intensity);
}

void robotLedSetBits(uint8_t bits) {
    iroblibState.bits = bits;
    irobledUpdate();
}

void robotLedOn(uint8_t led) {
    iroblibState.bits |= led;
    irobledUpdate();
}

void robotLedOff(uint8_t led) {
    iroblibState.bits &= ~led;
    irobledUpdate();
}

void robotLedToggle(uint8_t led) {
    iroblibState.bits ^= led;
    irobledUpdate();
}

void cmdLED1Set(uint8_t bit) {
    if (bit) {
        LED1On;
    } else {
        LED1Off;
    }
}

void cmdLED2Set(uint8_t bit) {
    if (bit) {
        LED2On;
    } else {
        LED2Off;
    }
}
