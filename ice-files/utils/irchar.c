#include "irchar.h"
#include "sensing.h"
#include "oi.h"

uint8_t _updateIR(uint8_t avg, uint8_t mask);

uint8_t redRunningAverage = 0;
uint8_t greenRunningAverage = 0;
uint8_t fieldRunningAverage = 0;

uint8_t region = IR_NONE;
uint8_t prevRegion = IR_NONE;

uint8_t irAny(void) {
    uint8_t ir = getSensorUint8(SenIRChar);
    if (ir == IR_NONE) return 0;
    return ((ir & IR_RESERVED) == IR_RESERVED);
}

uint8_t irAll(void) {
    return (getSensorUint8(SenIRChar) == IR_ALL);
}

uint8_t irCheck(uint8_t mask) {
    if (!irAny())   return 0;
    return (getSensorUint8(SenIRChar) & mask);
}

uint8_t irRed(void) {
    return irCheck(IR_MASK_RED_BUOY);
}

uint8_t irGreen(void) {
    return irCheck(IR_MASK_GREEN_BUOY);
}

uint8_t irForceField(void) {
    return irCheck(IR_MASK_FORCE_FIELD);
}

uint8_t _updateIR(uint8_t avg, uint8_t mask) {
    if (irCheck(mask)) {
        avg = (((uint16_t)avg) * (SMOOTHING_INTENSITY - 1) + 0xFF)\
              / SMOOTHING_INTENSITY;
    } else {
        avg /= SMOOTHING_INTENSITY;
    }
    return avg;
}

void updateIR(void) {
    redRunningAverage = _updateIR(redRunningAverage, IR_MASK_RED_BUOY);
    greenRunningAverage = _updateIR(greenRunningAverage, IR_MASK_GREEN_BUOY);
    fieldRunningAverage = _updateIR(fieldRunningAverage, IR_MASK_FORCE_FIELD);
    prevRegion = region;
    uint8_t red = (smoothRed() > 0x60);
    uint8_t green = (smoothGreen() > 0x60);
    uint8_t field = (smoothForceField() > 0x60);
    if (red && green && field) {
        region = IR_ALL;
    } else if (red && green) {
        region = IR_RED_AND_GREEN;
    } else if (red && field) {
        region = IR_RED_AND_FIELD;
    } else if (green && field) {
        region = IR_GREEN_AND_FIELD;
    } else if (red) {
        region = IR_RED_BUOY;
    } else if (green) {
        region = IR_GREEN_BUOY;
    } else if (field) {
        region = IR_FORCE_FIELD;
    } else {
        region = IR_NOWHERE;
    }
}

uint8_t smoothRed(void) {
    return redRunningAverage;
}

uint8_t smoothGreen(void) {
    return greenRunningAverage;
}

uint8_t smoothForceField(void) {
    return fieldRunningAverage;
}

uint8_t irRegion(void) {
    return region;
}

uint8_t irPrevRegion(void) {
    return prevRegion;
}
