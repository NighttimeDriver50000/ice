#include <stdint.h>
#include "timer.h"    // Declaration made available here


// Timer variables defined here
volatile uint32_t delayTimerCount = 0;   // Definition checked against declaration
volatile uint8_t  delayTimerRunning = 0; // Definition checked against declaration


// Chris -- moved to sensing.c
/*ISR(USART_RX_vect) {  //SIGNAL(SIG_USART_RECV) 
    // Serial receive interrupt to store sensor values

    // CSCE 274 students, I have only ever used this method 
    // when retrieving/storing a large amount of sensor data. 
    // You DO NOT need it for this assignment. If i feel it 
    // becomes relevant, I will show you how/when to use it.
}*/

//SIGNAL(SIG_OUTPUT_COMPARE1A)
ISR(TIMER1_COMPA_vect) {
    // Interrupt handler called every 1ms.
    // Decrement the counter variable, to allow delayMs to keep time.
    if(delayTimerCount != 0) {
        delayTimerCount--;
    } else {
        delayTimerRunning = 0;
    }
}

void setupTimer(void) {
    // Set up the timer 1 interupt to be called every 1ms.
    // It's probably best to treat this as a black box.
    // Basic idea: Except for the 71, these are special codes, for which details
    // appear in the ATMega168 data sheet. The 71 is a computed value, based on
    // the processor speed and the amount of "scaling" of the timer, that gives
    // us the 1ms time interval.
    TCCR1A = 0x00;
    // TCCR1B = 0x0C;
    TCCR1B = (_BV(WGM12) | _BV(CS12));
    OCR1A = 71;
    // TIMSK1 = 0x02;
    TIMSK1 = _BV(OCIE1A);
}

// Delay for the specified time in ms without updating sensor values
void delayMs(uint32_t time_ms) {
    delayTimerRunning = 1;
    delayTimerCount = time_ms;
    while(delayTimerRunning) ;
}

void delayMsFunc(uint32_t time_ms, void (*func)(void), uint16_t period_ms,
        uint16_t cutoff_ms) {
    // Initialize the conditions for the delay loop
    uint32_t lastExec = time_ms;
    uint32_t nextExec = lastExec - period_ms;
    // Start the timer
    delayTimerRunning = 1;
    delayTimerCount = time_ms;
    // Wait until the timer runs out (delayTimerCount decrements every ms)
    while(delayTimerRunning) {
        // If it's before the cutoff and time for the next execution
        if (delayTimerCount > cutoff_ms && delayTimerCount <= nextExec) {
            // Execute the function
            lastExec = delayTimerCount;
            nextExec = lastExec - period_ms;
            func();
        }
    }
}

void delayPredicateFunc(uint8_t (*pred)(void), void (*func)(void),
        uint16_t period_ms, uint16_t cutoff_ms) {
    // Initialize the conditions for the delay loop
    uint32_t lastExec = 0xFFFFFFFF;
    uint32_t nextExec = lastExec - period_ms;
    // Start the timer
    delayTimerRunning = 1;
    delayTimerCount = 0xFFFFFFFF;
    // Wait until the timer runs out (delayTimerCount decrements every ms)
    while(pred()) {
        // If it's before the cutoff and time for the next execution
        if (delayTimerCount > cutoff_ms && delayTimerCount <= nextExec) {
            // Execute the function
            lastExec = delayTimerCount;
            nextExec = lastExec - period_ms;
            func();
        }
    }
    delayMs(1);
}

void delayMsPredicateFunc(uint32_t time_ms, uint8_t (*pred)(void),
        void (*func)(void), uint16_t period_ms, uint16_t cutoff_ms) {
    // Initialize the conditions for the delay loop
    uint32_t lastExec = time_ms;
    uint32_t nextExec = lastExec - period_ms;
    // Start the timer
    delayTimerRunning = 1;
    delayTimerCount = time_ms;
    // Wait until the timer runs out (delayTimerCount decrements every ms)
    while(delayTimerRunning && pred()) {
        // If it's before the cutoff and time for the next execution
        if (delayTimerCount > cutoff_ms && delayTimerCount <= nextExec) {
            // Execute the function
            lastExec = delayTimerCount;
            nextExec = lastExec - period_ms;
            func();
        }
    }
}
