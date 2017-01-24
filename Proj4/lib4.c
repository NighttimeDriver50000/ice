#include "lib4.h"
#include "sensing.h"
#include "driving.h"
#include "oi.h"
#include "fixedqueue.h"
#include "irobserial.h"
#include "irchar.h"
#include "irobled.h"

#define PID_DT  (IROB_PERIOD_MS)

#define PRED    (irPrevRegion() & IR_MASK_RED_BUOY)
#define PGREEN  (irPrevRegion() & IR_MASK_GREEN_BUOY)
#define PFIELD  (irPrevRegion() & IR_MASK_FORCE_FIELD)
#define PANY    (PRED || PGREEN || PFIELD)
#define PALL    (PRED && PGREEN && PFIELD)
#define RED     (irRegion() & IR_MASK_RED_BUOY)
#define GREEN   (irRegion() & IR_MASK_GREEN_BUOY)
#define FIELD   (irRegion() & IR_MASK_FORCE_FIELD)
#define ANY     (RED || GREEN || FIELD)
#define ALL     (RED && GREEN && FIELD)

//#define CHARGING    (getSensorInt16(SenCurr1) >= CURRENTTHOLD)
#define CHARGING    (getSensorUint8(SenChAvailable))

int16_t utk = 0;
int16_t etk = 0;
int16_t etk_1 = 0;
int16_t esum = 0;
FixedQueue esumQueue = 0;

uint8_t bumpDrop = 0;
uint8_t prevBumpDrop = 0;

uint8_t started = 0;
uint8_t docking = 0;
uint8_t comingFromFront = 0;
uint8_t dockingFinal = 0;
uint8_t onDock = 0;

int16_t jimmyAngle = 0;

/**
 * initilaization function for a pid controller.
 */
void pidSetup(void) {
    while (esumQueue == 0) {
        esumQueue = newFixedQueue(PID_QSIZE);
    }
}
/**
 * A function to clear the value of the esumQueue
 * which is used in the integral calculations for
 * the PID controller.
 */
void pidCleanup(void) {
    if (esumQueue != 0) {
        freeFixedQueue(esumQueue);
        esumQueue = 0;
    }
}
/**
 * Takes the next input for the pid controler
 * utilizes constants:
 * PID_SET_POINT - the set point or goal
 * PID_KP - the multiplier for the current distance
 * PID_KI - the multiplier for the "integral" term
 * PID_KD - the multiplier for the "derivative" term
 * This then sets utk which is essentially the result
 * of this calculation
 * Important notes:
 * The derivative terms only look at this and the previous value
 * the integral term currently has no time mitigation and uses
 * a fixed size queue (now has damping based on size)
 * 
 * @param vtk the current value for the pid controller
 */
void pidStep(uint16_t vtk) {
    etk_1 = etk;
    etk = ((int16_t)vtk) - PID_SET_POINT;
    esum += etk;
    esum -= pushPop(esumQueue, etk);
    int16_t p = PID_KP*etk;
    int16_t i = PID_KI*esum*PID_DT / PID_QSIZE; // damping
    int16_t d = PID_KD*(etk-etk_1)/PID_DT;
    irobprintf("etk_1: %d\netk: %d\nesum: %d\nutk: %d\np: %d\ni: %d\nd: %d\n",
            etk_1, etk, esum, utk, p, i, d);
    utk = p + i + d;
}

/**
 * A drive function which utilizes the utk value (modified in pidStep)
 * this also utilizes 
 * DRIVE_DIVISOR - directly mitigates UTK
 * SPEED - the default speed
 */
void updateMotors(void) {
    int16_t deltaDrive = utk / DRIVE_DIVISOR;
    driveDirect(SPEED - deltaDrive, SPEED + deltaDrive);
}

// Do while turning after bump
void doWhileTurning(void) {
    updateSensors();
    uint8_t bumpDrop = getSensorUint8(SenBumpDrop);
    if (bumpDrop & MASK_WHEEL_DROP) {
        driveStop();
    }
    if (docking) {
        updateIR();
        dockingDiagnostics();
    }
}

void move(int16_t distance) {
    int16_t speed = docking ? DOCKING_SPEED : SPEED;
    driveDistanceTFunc(speed, distance, &doWhileTurning,
            UPDATE_SENSOR_DELAY_PERIOD, UPDATE_SENSOR_DELAY_CUTOFF);
}
void turn(int16_t radius, int16_t angle) {
    int16_t speed = docking ? DOCKING_SPEED : SPEED;
    driveAngleTFunc(speed, radius, angle, &doWhileTurning,
            UPDATE_SENSOR_DELAY_PERIOD, UPDATE_SENSOR_DELAY_CUTOFF);
}

uint8_t noBump(void) {
    return !(getSensorUint8(SenBumpDrop) & MASK_BUMP) && notCharging();
}
void jimmyBump(void) {
    // Drive forward until bump
    drivePredicateFunc(JIMMY_SPEED, RadStraight, &noBump,
            &doWhileTurning, UPDATE_SENSOR_DELAY_PERIOD,
            UPDATE_SENSOR_DELAY_CUTOFF);
}
uint8_t notCharging(void) {
    return !CHARGING;
}
void jimmyTurn(int16_t radius) {
    // Turn until charging
    driveAnglePFunc(JIMMY_SPEED, radius, jimmyAngle, &notCharging,
            &doWhileTurning, UPDATE_SENSOR_DELAY_PERIOD,
            UPDATE_SENSOR_DELAY_CUTOFF);
}
void jimmy(void) {
    // Increase angle every time
    jimmyAngle += JIMMY_ANGLE;
    // Turn right, then back to center
    jimmyTurn(RadCW);
    jimmyTurn(RadCCW);
    // Bump the dock
    jimmyBump();
    // Turn left, then back to center
    jimmyTurn(RadCCW);
    jimmyTurn(RadCW);
    // Bump the dock
    jimmyBump();
}

void dock(void) {
    if (!dockingFinal && !PGREEN && GREEN) {
        // Move an extra robot radius
        move(IROB_RAD_TURN);
        // Turn to line up
        turn(RadCW, FIELD_TURN);
        dockingFinal = 1;
    } else if (dockingFinal && RED && !GREEN) {
        // Course correction
        drive(DOCKING_SPEED, RadCCW);
    } else if (dockingFinal && !RED && GREEN) {
        // Course correction
        drive(DOCKING_SPEED, RadCW);
    } else {
        // Normally just go straight
        drive(DOCKING_SPEED, RadStraight);
    }
}

void dockingDiagnostics(void) {
    // Robot LEDs for IR fields
    robotLedSetBits(NEITHER_ROBOT_LED);
    powerLedSet(POWER_LED_ORANGE, 0);
    if (smoothRed() > 0x60)     robotLedOn(PLAY_ROBOT_LED);
    if (smoothGreen() > 0x60)   robotLedOn(ADVANCE_ROBOT_LED);
    if (irRegion() & IR_MASK_FORCE_FIELD) powerLedSet(POWER_LED_ORANGE, 0xFF);
    // Command module LEDs for charging.
    cmdLED1Set(0);
    cmdLED2Set(0);
    if (CHARGING) {
        cmdLED1Set(1);
    } else {
        cmdLED2Set(1);
    }
}

// Called by irobPeriodic
void iroblifePeriodic(void) {
    // Get bump & wheel drop sensor
    prevBumpDrop = bumpDrop;
    bumpDrop = getSensorUint8(SenBumpDrop);
    // IR
    updateIR();
    dockingDiagnostics();
    if (onDock) {
        // Final connection on dock
        if (CHARGING) {
            driveStop();
        } else {
            jimmy();
        }
    } else if (bumpDrop & MASK_WHEEL_DROP) {
        // Cliff
        driveStop();
    } else if (bumpDrop & MASK_BUMP) {
        if (dockingFinal) {
            // We are now on the dock
            driveStop();
            onDock = 1;
        } else {
            // Turn until no longer bumping
            drive(SPEED, RadCCW);
            started = 1;
        }
    } else if (prevBumpDrop & MASK_BUMP) {
        turn(RadCCW, OVERTURN);
    } else if (docking) {
        dock();
    } else if (!docking && FIELD) {
        // Begin docking
        // If we were already in red, we're coming from front.
        comingFromFront = PRED;
        turn(RadCCW, FIELD_TURN);
        if (!comingFromFront) {
            // Turn again to be perpendicular
            move(FIELD_CLEARANCE);
            turn(RadCW, FIELD_TURN);
        }
        // We are now docking
        docking = 1;
    } else if (!started) {
        // Go straight until wall
        drive(SPEED, RadStraight);
    } else {
        // PID
#ifdef LOG_OVER_USB
        setSerialDestination(SERIAL_USB);
#endif
        uint16_t wallSignal = getSensorUint16(SenWallSig1);
        pidStep(wallSignal);
#ifdef LOG_OVER_USB
        irobprintf("wallSignal: %u\ndeltaDrive: %d\n\n", utk / DRIVE_DIVISOR);
        setSerialDestination(SERIAL_CREATE);
#endif
        updateMotors();
    }
}
