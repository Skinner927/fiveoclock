#ifndef INCLUDE_SERIAL_HELP_H
#define INCLUDE_SERIAL_HELP_H

#include <Arduino.h>
#include "SerialHelp.h"
#include "SharedESPMessages.h"

void clearSerial(HardwareSerial &serial);

void sendAndWait(HardwareSerial &serial, const char *cmd);

char serialWait(HardwareSerial &serial, const char *waitFor, uint8_t waitForLength, unsigned long timeout_in_ms);
char serialWait(HardwareSerial &serial, const char waitFor, unsigned long timeout_in_ms);

#endif