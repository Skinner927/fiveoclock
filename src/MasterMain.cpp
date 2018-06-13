#ifdef MASTER
#include <Arduino.h>
#include "SerialHelp.h"
#include "SharedESPMessages.h"

#define DS_LOG(...) Serial.println(__VA_ARGS__)
#define DS_LOG_l(...) Serial.print(__VA_ARGS__)

#define RESET_BUTTON 52

void wifiHandshake()
{
    Serial.println("Waiting for WiFi handshake");
    // Wait for our wifi to be ready
    serialWait(Serial1, WIFI_READY, 0);
    // Send confirmation
    Serial1.print(WIFI_READY);
}

void setup()
{
    pinMode(RESET_BUTTON, INPUT_PULLUP);

    Serial.begin(9600);
    Serial1.begin(9600);

    wifiHandshake();

    // IF the reset button is being held down, reset wifi
    // LOW read means button is pushed
    if (!digitalRead(RESET_BUTTON))
    {
        DS_LOG("Resetting WiFi");
        Serial1.print(RESET_WIFI_SETTINGS);
        serialWait(Serial1, RESET_WIFI_SETTINGS, 0);
        // TODO: Restart ESP and re-handshake
    }

    // Connect to WiFi
    bool wifiConnected = false;
    const uint8_t listenCount = 4;
    char listenFor[listenCount] = {
        CONNECT_WIFI_SUCCESS,
        CONNECT_WIFI_FAILURE,
        CONNECT_WIFI_CONFIG_IP,
        CONNECT_WIFI_BAD_CREDS};
    char response = '\0';
    while (!wifiConnected)
    {
        DS_LOG("Waiting for WiFi response.");
        // Send connect to WiFi command
        Serial1.println(CONNECT_WIFI);

        response = serialWait(Serial1, listenFor, listenCount, 0);
        switch (response)
        {
        case CONNECT_WIFI_SUCCESS:
            DS_LOG("WiFi Connected");
            wifiConnected = true;
            break;
        case CONNECT_WIFI_FAILURE:
            // If connection fails, ESP will restart
            DS_LOG("WIFI Failed! Restarting handshake.");
            wifiHandshake();
            break;
        case CONNECT_WIFI_BAD_CREDS:
            DS_LOG("Bad WiFi SSID or password. Try again!");
            break;
        case CONNECT_WIFI_CONFIG_IP:
            DS_LOG("WiFi needs to be configured.");
            DS_LOG_l("WiFi IP: ");
            DS_LOG(Serial1.readStringUntil('\n'));
            break;
        default:
            DS_LOG_l("Unknown command: ");
            DS_LOG(response);
            break;
        }
    }
    // Clear any lingering buffer
    clearSerial(Serial1);

    //Serial1.println(RESET_WIFI_SETTINGS);
    //serialWait(Serial1, RESET_WIFI_SETTINGS, 0);
}

void loop()
{
    DS_LOG("LOOP");
    delay(1000);
}


#endif