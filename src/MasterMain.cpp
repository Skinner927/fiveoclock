#ifdef MASTER
#include <Arduino.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include "SerialHelp.h"
#include "SharedESPMessages.h"

#define DS_LOG(...) Serial.println(__VA_ARGS__)
#define DS_LOG_l(...) Serial.print(__VA_ARGS__)

// Serial port for WiFi chip
#define NET Serial1

/******* PINS *******/
// https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/#_interrupt_numbers
// 2 is PWM pin 2 on Mega
#define RCT_INTERRUPT_PIN 2
#define RESET_BUTTON 52

// Number of clocks we'll be printing to
#define clock_count 5
int offsets[clock_count] = {0};

const String web_start_flag = "!$$%%\n";

RtcDS3231<TwoWire> Rtc(Wire);
volatile bool interruptRefreshTime = false;

void wifiHandshake()
{
    Serial.println("Waiting for WiFi handshake");
    // Wait for our wifi to be ready
    serialWait(NET, WIFI_READY, 0);
    // Send confirmation
    NET.print(WIFI_READY);
}

void rtcInterruptHandler()
{
    interruptRefreshTime = true;
}

// TODO DUMP THIS
#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(HardwareSerial &serial, const RtcDateTime &dt)
{
    char datestring[20];

    snprintf_P(datestring,
               countof(datestring),
               PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
               dt.Month(),
               dt.Day(),
               dt.Year(),
               dt.Hour(),
               dt.Minute(),
               dt.Second());
    serial.println(datestring);
}

bool clearBufferUntilAfter(HardwareSerial &serial, const String boundary, int timeout)
{
    char firstChar = boundary[0];
    uint8_t max = boundary.length() - 1;

    while(serial.available() > 0) {
        DS_LOG_l((char)NET.read());
    }

    while(true) 
    {
        DS_LOG_l("Looking for");
        DS_LOG(firstChar);
        // wait for the first character
        char r = serialWait(serial, firstChar, timeout);
        if (r == '\0') {
            // Timeout
            return false;
        }

        DS_LOG("Found start");
        DS_LOG("Expected   Found");
        bool badChar = false;
        for (uint8_t j = 1; j < max; j++) {
            // Wait for next character
            while(serial.available() < 1) {
                delay(1);
            }
            char c = serial.read();
            DS_LOG_l(c);
            DS_LOG_l("  ");
            DS_LOG(boundary[j]);
            if (c != boundary[j]) {
                DS_LOG("MISSED IT");
                badChar = true;
                break;
            }
            DS_LOG("KEEP GOING!");
        }
        // We did it!
        if (!badChar) {
            return true;
        }
        // Otherwise we failed, loop around and wait for another sequence
    }
}

// pull time from server
void refreshTime()
{
    DS_LOG("Getting time from Internet.");
    clearSerial(NET);
    NET.print(GET_TIME);
    DS_LOG("Waiting for server to respond");
    while(NET.available()) {
        DS_LOG_l((char)NET.read());
    }
    // This dumps the HTTP headers
    if (!clearBufferUntilAfter(NET, web_start_flag, 10000)) {
        DS_LOG("Web request failed, not updating time.");
        return;
    }
    // Set time
    char epochStr[17] = {'\0'};
    serialReadLine(NET).toCharArray(epochStr, 17);
    uint32_t epoch = strtoul(epochStr, NULL, 10);
    DS_LOG_l("Epoch ");
    DS_LOG(epoch);
    RtcDateTime compiled = RtcDateTime(epoch);
    Rtc.SetDateTime(compiled);

    // Update offsets
    for (uint8_t i = 0; i < clock_count; i++)
    {
        int offset = serialReadLine(NET).toInt();
        DS_LOG_l("Offset ");
        DS_LOG_l(i);
        DS_LOG_l(" ");
        DS_LOG(offset);
        offsets[i] = offset;
    }
    DS_LOG("Time updated");
}

void setup()
{
    pinMode(RESET_BUTTON, INPUT_PULLUP);

    Serial.begin(9600);
    NET.begin(9600);

    wifiHandshake();

    // IF the reset button is being held down, reset wifi
    // LOW read means button is pushed
    if (!digitalRead(RESET_BUTTON))
    {
        DS_LOG("Resetting WiFi");
        NET.print(RESET_WIFI_SETTINGS);
        serialWait(NET, RESET_WIFI_SETTINGS, 0);
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
        NET.println(CONNECT_WIFI);

        response = serialWait(NET, listenFor, listenCount, 0);
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
            DS_LOG(serialReadLine(NET));
            break;
        default:
            DS_LOG_l("Unknown command: ");
            DS_LOG(response);
            break;
        }
    }
    // Clear any lingering buffer
    clearSerial(NET);

    // Setup the clock
    DS_LOG("Starting clock");
    Rtc.Begin();
    if (!Rtc.GetIsRunning())
    {
        Rtc.SetIsRunning(true);
    }
    if (!Rtc.IsDateTimeValid())
    {
        DS_LOG("Bad battery in RTC module");
    }
    refreshTime();

    // Turn extras off
    Rtc.Enable32kHzPin(false);
    // Turn on pullup on interrupt pin
    pinMode(RCT_INTERRUPT_PIN, INPUT_PULLUP);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeAlarmTwo);
    // Ditch any old alarm state
    Rtc.LatchAlarmsTriggeredFlags();

    // Create our alarm that will let us know when we're
    // at the top of the hour
    DS3231AlarmTwo alarm2(0, 0, 0, DS3231AlarmTwoControl_MinutesMatch);
    Rtc.SetAlarmTwo(alarm2);

    // Hook it up
    attachInterrupt(digitalPinToInterrupt(RCT_INTERRUPT_PIN), rtcInterruptHandler, FALLING);

    DS_LOG("Setup Finished. Starting Loop");
}

RtcDateTime now;
uint32_t lastEpochWithoutSeconds = 0;
void loop()
{
    if (interruptRefreshTime)
    {
        DS_LOG("INTERRUPT");
        // Reset the interrupt
        interruptRefreshTime = false;
        Rtc.LatchAlarmsTriggeredFlags();
        refreshTime();
    }
    now = Rtc.GetDateTime();
    uint32_t nowWithoutSeconds = now.TotalSeconds() - now.Second();
    if (lastEpochWithoutSeconds != nowWithoutSeconds) {
        lastEpochWithoutSeconds = nowWithoutSeconds;

        // AT this point we'd update the clocks
        for (uint8_t i = 0; i < clock_count; i++) {
            RtcDateTime myTime = RtcDateTime(now.TotalSeconds() + offsets[i]);
            DS_LOG_l("Clock ");
            DS_LOG_l(i);
            DS_LOG(":");
            printDateTime(Serial, myTime);
        }    
    }

    // Sleep for a second every loop
    // At worst our clock is 1.99 seconds off
    delay(1000);
}

#endif