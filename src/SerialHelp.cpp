#include "SerialHelp.h"

void clearSerial(HardwareSerial &serial)
{
    while (serial.available() > 0)
        serial.read();
}

void sendAndWait(HardwareSerial &serial, const char *cmd)
{
    clearSerial(serial);
    serial.print(cmd);
    while (serial.available() < 1)
        delay(100);
}

char serialWait(HardwareSerial &serial, const char *waitFor, uint8_t waitForLength, unsigned long timeout_in_ms)
{
    unsigned long start = millis();
    while (timeout_in_ms == 0 || millis() - start < timeout_in_ms)
    {
        if (serial.available() < 1)
        {
            delay(100);
            continue;
        }
        while (serial.available() > 0)
        {
            char read = serial.read();
            for (uint8_t i = 0; i < waitForLength; i++)
            {
                if (read == waitFor[i])
                    return read;
            }
        }
    }
    return '\0';
}

char serialWait(HardwareSerial &serial, const char waitFor, unsigned long timeout_in_ms)
{
    char waitArray[1] = {waitFor};
    return serialWait(serial, waitArray, 1, timeout_in_ms);
}

String serialReadLine(HardwareSerial &serial) {
    String output;
    while(serial.available() < 1) {
        delay(10);
    }
    while(serial.available() > 0) {
        char c = serial.read();
        if (c == '\n') {
            return output;
        }
        output += c;
    }
    // Should never get here
    return output;
}
