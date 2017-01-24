#ifndef SENSING_H
#define SENSING_H

#include <stdint.h>

#define UPDATE_SENSOR_DELAY_PERIOD      (1)
#define UPDATE_SENSOR_DELAY_CUTOFF      (10)


#define PACKET_BUMPS_AND_WHEEL_DROPS    (7)
#define MASK_WHEEL_DROP_CASTER          (1 << 4)
#define MASK_WHEEL_DROP_LEFT            (1 << 3)
#define MASK_WHEEL_DROP_RIGHT           (1 << 2)
#define MASK_WHEEL_DROP                 (0x1C)
#define MASK_BUMP_LEFT                  (1 << 1)
#define MASK_BUMP_RIGHT                 (1 << 0)
#define MASK_BUMP                       (0x03)

#define PACKET_BUTTONS                  (18)
#define MASK_BTN_ADVANCE                (1 << 2)
#define MASK_BTN_PLAY                   (1 << 0)

#define IR_LEFT                         (129)
#define IR_FORWARD                      (130)
#define IR_RIGHT                        (131)

#define PACKET_ALL                      (6)

//! Request a sensor packet. \see read1ByteSensorPacket(uint8_t)
/*!
 *  \deprecated {
 *      This uses the old, non-USART-based way of retrieving sensor data.
 *  }
 */
void requestPacket(uint8_t packetId);

//! Read in a 1-byte sensor packet.
/*!
 *  \deprecated {
 *      This uses the old, non-USART-based way of retrieving sensor data.
 *  }
 *  
 *  What is a sensor packet? A byte (or bytes) containing data from a set of
 *  sensors, often shifted and ORed together. See the Create Open Interface
 *  documentation for more.
 *
 *  Currently Available Sensor Packets  (v = read1ByteSensorPacket(packetId)):
 *      Bumps and Wheel Drops   (packetId = PACKET_BUMPS_AND_WHEEL_DROPS):
 *          Caster Drop         (v & MASK_WHEEL_DROP_CASTER)
 *          Left Wheel Drop     (v & MASK_WHEEL_DROP_LEFT)
 *          Right Wheel Drop    (v & MASK_WHEEL_DROP_RIGHT)
 *          Any Wheel Drop      (v & MASK_WHEEL_DROP)
 *          Left Bumper         (v & MASK_BUMP_LEFT)
 *          Right Bumper        (v & MASK_BUMP_RIGHT)
 *          Either Bumper       (v & MASK_BUMPER)
 *      Create Buttons          (packetId = PACKET_BUTTONS):
 *          Advance Button      (v & MASK_BTN_ADVANCE)
 *          Play Button         (v & MASK_BTN_PLAY)
 *
 *  \param packetId     The ID of the packet to retrieve, as defined by the
 *                      Create Open Interface.
 */
uint8_t read1ByteSensorPacket(uint8_t packetId);

//! Request all packets (will be retrieved by USART)
void updateSensors(void);

//! Wait for all packets to be recieved by USART
void waitForSensors(void);

//! delayMs that updates sensors
void delayAndUpdateSensors(uint32_t time_ms);

//! Get an unsigned 1-byte sensor value
uint8_t getSensorUint8(uint8_t index);

//! Get a signed 1-byte sensor value
int8_t getSensorInt8(uint8_t index);

//! Get an unsigned 2-byte sensor value, indexed by the more significant
//! (lower index) byte
uint16_t getSensorUint16(uint8_t index1);

//! Get a signed 2-byte sensor value, indexed by the more significant
//! (lower index) byte
int16_t getSensorInt16(uint8_t index1);

#endif
