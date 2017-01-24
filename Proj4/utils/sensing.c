#include <stdint.h>
#include "sensing.h"
#include "cmod.h"
#include "timer.h"
#include "oi.h"
#include "irobserial.h"

volatile uint8_t usartActive = 0;
volatile uint8_t sensorIndex = 0;
volatile uint8_t sensorBuffer[Sen6Size];
volatile uint8_t sensors[Sen6Size];

void requestPacket(uint8_t packetId) {
    byteTx(CmdSensors);
    byteTx(packetId);
}

uint8_t read1ByteSensorPacket(uint8_t packetId) {
    // Send the packet ID
    requestPacket(packetId);
    // Read the packet byte
    return byteRx();
}

ISR(USART_RX_vect) {
    // Cache the retrieved byte
    uint8_t tmpUDR0;
    tmpUDR0 = UDR0;
    // Don't do anything if we're not looking
    if (usartActive) {
        if (getSerialDestination() == SERIAL_CREATE) {
            // New sensor data from the create
            sensorBuffer[sensorIndex++] = tmpUDR0;
        } else {
            // Probably input from the computer, loop old values around
            sensorBuffer[sensorIndex] = sensors[sensorIndex];
            sensorIndex++;
        }
        if (sensorIndex >= Sen6Size) {
            // Reached end of sensor packet
            usartActive = 0;
        }
    }
}

void updateSensors(void) {
    // Don't do anything if sensors are still coming in
    if (!usartActive) {
        uint8_t i;
        for (i = 0; i < Sen6Size; i++) {
            // Copy in the sensor buffer so the most recent data is available
            sensors[i] = sensorBuffer[i];
        }
        // Bookkeeping
        sensorIndex = 0;
        usartActive = 1;
        // Request all sensor data
        requestPacket(PACKET_ALL);
    }
}

void waitForSensors(void) {
    // Sensors data are coming in if usartActive is true
    while(usartActive);
}

void delayAndUpdateSensors(uint32_t time_ms) {
    // Update sensors while waiting
    delayMsFunc(time_ms, &updateSensors, 1, UPDATE_SENSOR_DELAY_CUTOFF);
}

uint8_t getSensorUint8(uint8_t index) {
    // Already in the right format
    return sensors[index];
}

int8_t getSensorInt8(uint8_t index) {
    uint8_t x = getSensorUint8(index);
    // Convert to signed; not implementation-dependent, and optimizes away
    return x < (1 << 7) ? x : x - (1 << 8);
}

uint16_t getSensorUint16(uint8_t index1) {
    // Combine msB and lsB
    return (sensors[index1] << 8) | sensors[index1 + 1];
}

int16_t getSensorInt16(uint8_t index1) {
    uint16_t x = getSensorUint16(index1);
    // Convert to signed; more opaque hex values b/c avr complains for 1 << 16
    return x < 0x8000 ? x : x - 0x10000;
}
