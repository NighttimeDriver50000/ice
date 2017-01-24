#include "iroblife.h"
#include "sensing.h"

#include "lib4.h"

int main(void) {
    // Submit to iroblife
    setIrobInitImpl(&pidSetup);
    setIrobPeriodicImpl(&iroblifePeriodic);
    setIrobEndImpl(&pidCleanup);

    // Initialize the Create
    irobInit();

    // Infinite operation loop
    for(;;) {
        // Periodic execution
        irobPeriodic();
       
        // Delay for the loop; one second
        delayAndUpdateSensors(IROB_PERIOD_MS);
    }
}

