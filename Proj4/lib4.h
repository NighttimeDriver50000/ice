#ifndef LIB2A_H
#define LIB2A_H

#include <stdint.h>

// Delay constant
#define IROB_PERIOD_MS  (32)

// PID settings
#define PID_SET_POINT   (32)
#define PID_KP          (256)
#define PID_KI          (1)
#define PID_KD          (64)
#define DRIVE_DIVISOR   (128)
#define PID_QSIZE       (128)

// Charging current threshold
#define CURRENTTHOLD    (-150)

// # Drive settings #
// Speed settings
#define SPEED           (100)
#define DOCKING_SPEED   (50)
#define JIMMY_SPEED     (30)
// Angle settings
#define OVERTURN        (10)
#define FIELD_TURN      (90)
#define FRONT_TURN      (60)
#define JIMMY_ANGLE     (10)
// Distance settings
#define FIELD_CLEARANCE (300)
#define IROB_RAD_TURN   (150)

//#define LOG_OVER_USB

void pidSetup(void);

void pidCleanup(void);

void pidStep(uint16_t vtk);

void updateMotors(void);

void doWhileTurning(void);

void move(int16_t distance);
void turn(int16_t radius, int16_t angle);

uint8_t noBump(void);
void jimmyBump(void);
uint8_t notCharging(void);
void jimmyTurn(int16_t radius);
void jimmy(void);

void dock(void);
void dockingDiagnostics(void);

//! Called by irobPeriodic
void iroblifePeriodic(void);

#endif
