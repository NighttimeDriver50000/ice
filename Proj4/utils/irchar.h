#ifndef IRCHAR_H
#define IRCHAR_H

#include <stdint.h>

// 1-0x101
#define SMOOTHING_INTENSITY     (16)

#define IR_RESERVED         (240)
#define IR_NOWHERE          (IR_RESERVED)
#define IR_RED_BUOY         (248)
#define IR_GREEN_BUOY       (244)
#define IR_FORCE_FIELD      (242)
#define IR_RED_AND_GREEN    (252)
#define IR_RED_AND_FIELD    (250)
#define IR_GREEN_AND_FIELD  (246)
#define IR_ALL              (254)
#define IR_NONE             (255)

#define IR_MASK_RED_BUOY    (IR_RED_BUOY ^ IR_RESERVED)
#define IR_MASK_GREEN_BUOY  (IR_GREEN_BUOY ^ IR_RESERVED)
#define IR_MASK_FORCE_FIELD (IR_FORCE_FIELD ^ IR_RESERVED)

uint8_t irAny(void);
uint8_t irAll(void);
uint8_t irCheck(uint8_t mask);
uint8_t irRed(void);
uint8_t irGreen(void);
uint8_t irForceField(void);

void updateIR(void);
uint8_t smoothRed(void);
uint8_t smoothGreen(void);
uint8_t smoothForceField(void);

uint8_t irRegion(void);
uint8_t irPrevRegion(void);

#endif
