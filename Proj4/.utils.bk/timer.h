#ifndef INCLUDE_TIMER_H
#define INCLUDE_TIMER_H

#include <avr/io.h>
#include <avr/interrupt.h>

// Interrupts.
ISR(TIMER1_COMPA_vect);

// Timer functions
void setupTimer(void);
void delayMs(uint32_t time_ms);

// Declaration of timer variables
extern volatile uint32_t delayTimerCount;
extern volatile uint8_t  delayTimerRunning;

//! Wait milliseconds, execute a function periodically.
/*! 
 *  Executes a function at an interval until a cutoff has passed, returning
 *  after a total number of milliseconds have passed.
 *
 *  \param time_ms      The total number of seconds to wait.
 *  \param func         The function to execute periodically.
 *  \param period_ms    The interval to execute the function.
 *  \param cutoff_ms    The number of milliseconds before the end to stop
 *                      attempting to start the function.
 */
void delayMsFunc(uint32_t time_ms, void (*func)(void), uint16_t period_ms,
        uint16_t cutoff_ms);

void delayPredicateFunc(uint8_t (*pred)(void), void (*func)(void),
        uint16_t period_ms, uint16_t cutoff_ms);

void delayMsPredicateFunc(uint32_t time_ms, uint8_t (*pred)(void),
        void (*func)(void), uint16_t period_ms, uint16_t cutoff_ms);

#endif
