#ifdef ESP_SLAVE
#include <Arduino.h>
#include <DNSServer.h>        //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h> //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>      //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include "SerialHelp.h"
#include "SharedESPMessages.h"

#define WEB_HOST "192.168.1.167"
#define WEB_PATH "/"
#define WEB_PORT 8080

WiFiManager manager;

// Called when we switch into config mode
void configModeCallback(WiFiManager *myManager)
{
    Serial.print(CONNECT_WIFI_CONFIG_IP);
    Serial.println(WiFi.softAPIP());
}

void badCredentialsCallback(WiFiManager *myManager)
{
    Serial.print(CONNECT_WIFI_BAD_CREDS);
}

void getTime()
{
    WiFiClient client;
    if (!client.connect(WEB_HOST, WEB_PORT))
    {
        Serial.println("FAIL");
        return;
    }
    client.println("GET " WEB_PATH " HTTP/1.1");
    client.println("Host: " WEB_HOST);
    client.println("Connection: close");
    client.println();
    delay(10);

    // Wait up to 5 seconds for server to respond
    uint16_t count = 0;
    while(!client.available()) {
        if (count >= 500) {
            // abort
            Serial.println("FAIL");
            return;
        }
        count++;
        delay(10);
    }
    while(client.available()) {
        Serial.print((char)client.read());
    }
    Serial.println();
}

void setup()
{
    Serial.begin(9600); // start serial for i/o
    WiFi.persistent(true);
    manager.setConfigPortalTimeout(0); // no timeout
    manager.setConnectTimeout(60);
    manager.setDebugOutput(false);
    manager.setAPCallback(configModeCallback);
    manager.setBadCredentialsCallback(badCredentialsCallback);

    // Take a deep breath
    delay(1000);

    // Wait for master to see us
    Serial.print(WIFI_READY);
    char response = '\0';
    while (true)
    {
        response = serialWait(Serial, WIFI_READY, 3000);
        if (response == '\0') // Timeout
            Serial.print(WIFI_READY);
        else
            break;
    }
}

bool result;
void loop()
{
    while (Serial.available() > 0)
    {
        char cmd = Serial.read();

        switch (cmd)
        {
        case CONNECT_WIFI:
            result = manager.autoConnect("ClockSetup");
            clearSerial(Serial);
            if (result)
            {
                Serial.print(CONNECT_WIFI_SUCCESS);
            }
            else
            {
                Serial.print(CONNECT_WIFI_FAILURE);
                delay(500);
                // TODO: At this point because there's a bug
                // with ESP hardware, Arduino should toggle us.
                // Under further examination I can't even get this
                // error state to trigger so we'll just tell the
                // user to hard reset the Clock if this ever
                // happens.
                ESP.restart();
                delay(2000);
            }
            break;
        case RESET_WIFI_SETTINGS:
            manager.resetSettings();
            Serial.print(RESET_WIFI_SETTINGS);
            break;
        case GET_TIME:
            getTime();
            break;
        }
    }
    delay(100);
}
#endif