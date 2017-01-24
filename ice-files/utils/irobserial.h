#ifndef IROBSERIAL_H
#define IROBSERIAL_H

#include <stdint.h>
#include <stdarg.h>

#define SERIAL_CREATE       (1)
#define SERIAL_USB          (2)
#define SERIAL_SWITCHING    (0xFF)

#define PRINTF_BUFFER_SIZE  (0xFF)

//! Set the serial output (CREATE or USB)
//! Takes some time.
void setSerialDestination(uint8_t dest);

//! Get the serial output (CREATE or USB)
uint8_t getSerialDestination(void);

//! Print a string
void irobprint(char* str);

//! Print a formatted string (Max length: 255 bytes)
void irobprintf(const char* format, ...);

//! Print a formatted string (for strings longer than 255 bytes)
void irobnprintf(uint16_t size, const char* format, ...);

#endif
