#ifndef IROBLED_H
#define IROBLED_H

#include <stdint.h>

// Colors for the power led.
#define POWER_LED_GREEN   (0x00)
#define POWER_LED_ORANGE  (0x40)
#define POWER_LED_RED     (0xFF)

// Bits for the other leds.
#define NEITHER_ROBOT_LED (0x00)
#define PLAY_ROBOT_LED    (0x02)
#define ADVANCE_ROBOT_LED (0x08)
#define BOTH_ROBOT_LED    (0x0A)

//! Send an led command to the Create.
void irobledCmd(uint8_t bits, uint8_t color, uint8_t intensity);
//! Update the leds. Probably won't have to use.
void irobledUpdate(void);
//! Initialize the leds to red for power and off for the others.
void irobledInit(void);

//! Set the color and intensity of the power led.
void powerLedSet(uint8_t color, uint8_t intensity);

// Functions for modifying one or both of the other leds.
void robotLedSetBits(uint8_t bits);
void robotLedOn(uint8_t led);
void robotLedOff(uint8_t led);
void robotLedToggle(uint8_t led);

void cmdLED1Set(uint8_t bit);
void cmdLED2Set(uint8_t bit);

#endif
