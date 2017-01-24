#include <stdint.h>
#include "driving.h"
#include "oi.h"
#include "cmod.h"
#include "timer.h"

// Weird constants because squeezing out precision
#define PIe5            314159
#define TENTH_RADIUS    13

// # BASIC COMMANDS #

void driveDirect(uint16_t left, uint16_t right) {
    // Send the direct drive command to the Create
    byteTx(CmdDriveWheels);
    uint16Tx(right);
    uint16Tx(left);
}

void drive(int16_t velocity, int16_t radius) {
    // Send the start driving command to the Create
    byteTx(CmdDrive);
    uint16Tx(velocity);
    uint16Tx(radius);
    /*byteTx((uint8_t)((velocity >> 8) & 0x00FF));
    byteTx((uint8_t)(velocity & 0x00FF));
    byteTx((uint8_t)((radius >> 8) & 0x00FF));
    byteTx((uint8_t)(radius & 0x00FF));*/
}

void driveStop(void) {
    drive(0, RadStraight);
}


// # OPCODE-BASED COMMANDS #

void driveDistanceOp(int16_t velocity, int16_t distance) {
    // Start driving
    drive(velocity, RadStraight);
    // Halt execution of new commands on the Create until reached distance
    byteTx(WaitForDistance);
    uint16Tx(distance);
    /*byteTx((uint8_t)((distance >> 8) & 0x00FF));
    byteTx((uint8_t)(distance & 0x00FF));*/
    // Stop the Create
    driveStop();
}

void driveAngleOp(int16_t velocity, int16_t radius, int16_t angle) {
    // Wait for angle opcode compatibility
    if (radius == RadCW) {
        angle = -angle;
    }
    // Start driving
    drive(velocity, radius);
    // Halt execution of new commands on the Create until reached angle
    byteTx(WaitForAngle);
    uint16Tx(angle);
    /*byteTx((uint8_t)((angle >> 8) & 0x00FF));
    byteTx((uint8_t)(angle & 0x00FF));*/
    // Stop the Create
    driveStop();
}


// # TIMER-BASED COMMANDS #

void driveDistanceTFunc(int16_t velocity, int16_t distance, void (*func)(void),
        uint16_t period_ms, uint16_t cutoff_ms) {
    // Calculate the delay
    uint32_t time_ms = (1000 * (uint32_t)distance) / (uint32_t)velocity;
    // Start driving
    drive(velocity, RadStraight);
    // Wait delay
    delayMsFunc(time_ms, func, period_ms, cutoff_ms);
    // Stop the Create
    driveStop();
}

void driveAngleTFunc(int16_t velocity, int16_t radius, int16_t angle,
        void (*func)(void), uint16_t period_ms, uint16_t cutoff_ms) {
    // Calculate the delay
    uint32_t time_ms = (PIe5 * TENTH_RADIUS * (uint32_t)angle)
        / (1800 * (uint32_t)velocity);
    // Start driving
    drive(velocity, radius);
    // Wait delay
    delayMsFunc(time_ms, func, period_ms, cutoff_ms);
    // Stop the Create
    driveStop();
}

// # PREDICATE-BASED COMMANDS #

void drivePredicateFunc(int16_t velocity, int16_t radius,
        uint8_t (*pred)(void), void (*func)(void), uint16_t period_ms,
        uint16_t cutoff_ms) {
    // Start driving
    drive(velocity, radius);
    // Wait
    delayPredicateFunc(pred, func, period_ms, cutoff_ms);
    // Stop the Create
    driveStop();
}


void driveDistancePFunc(int16_t velocity, int16_t distance,
        uint8_t (*pred)(void), void (*func)(void), uint16_t period_ms,
        uint16_t cutoff_ms) {
    // Calculate the delay
    uint32_t time_ms = (1000 * (uint32_t)distance) / (uint32_t)velocity;
    // Start driving
    drive(velocity, RadStraight);
    // Wait delay
    delayMsPredicateFunc(time_ms, pred, func, period_ms, cutoff_ms);
    // Stop the Create
    driveStop();
}

void driveAnglePFunc(int16_t velocity, int16_t radius, int16_t angle,
        uint8_t (*pred)(void), void (*func)(void), uint16_t period_ms,
        uint16_t cutoff_ms) {
    // Calculate the delay
    uint32_t time_ms = (PIe5 * TENTH_RADIUS * (uint32_t)angle)
        / (1800 * (uint32_t)velocity);
    // Start driving
    drive(velocity, radius);
    // Wait delay
    delayMsPredicateFunc(time_ms, pred, func, period_ms, cutoff_ms);
    // Stop the Create
    driveStop();
}
