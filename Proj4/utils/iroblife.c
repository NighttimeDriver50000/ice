#include <stdlib.h>
#include <stdint.h>
#include "iroblife.h"

#include "timer.h"
#include "cmod.h"
#include "iroblib.h"
#include "oi.h"

#include "sensing.h"
#include "irobled.h"
#include "driving.h"
#include "irobserial.h"

void irobImplNull(void) {
}

void (*irobInitImpl)(void) = &irobImplNull;
void (*irobPeriodicImpl)(void) = &irobImplNull;
void (*irobEndImpl)(void) = &irobImplNull;

void setIrobInitImpl(void (*func)(void)) {
    irobInitImpl = func;
}

void setIrobPeriodicImpl(void (*func)(void)) {
    irobPeriodicImpl = func;
}

void setIrobEndImpl(void (*func)(void)) {
    irobEndImpl = func;
}

void irobInit(void) {
    // Set up Create and module
    initializeCommandModule();
    // Set Create as default serial destination
    setSerialDestination(SERIAL_CREATE);
    
    // Is the Robot on
    powerOnRobot();
    // Start the create
    byteTx(CmdStart);
    // Set the baud rate for the Create and Command Module
    baud(Baud57600);
    // Define some songs so that we know the robot is on.
    defineSongs();
    // Deprecated form of safe mode. I use it because it will
    // turn of all LEDs, so it's essentially a reset.
    byteTx(CmdControl);
    // We are operating in FULL mode.
    byteTx(CmdFull);

    // Make sure the robot stops. 
    // As a precaution for the robot and your grade.
    driveStop();

    // Play the reset song and wait while it plays.
    byteTx(CmdPlay);
    byteTx(RESET_SONG);
    delayMs(750);

    // Turn the power button on to orange.
    irobledInit();

    // Call the user's init function
    irobInitImpl();
}

void irobPeriodic(void) {
    // Call the user's periodic function
    irobPeriodicImpl();
    // Exit if the black button on the command module is pressed.
    if(UserButtonPressed) {
        irobEnd();
    }
}

void irobEnd(void) {
    // Call the user's end function
    irobEndImpl();
    // Stop the Create
    driveStop();
    // Power off the Create
    powerOffRobot();
    // Exit the program
    exit(1);
}
