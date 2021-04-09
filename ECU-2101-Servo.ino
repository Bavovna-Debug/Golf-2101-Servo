#include <Wire.h>
#include "API.h"
#include "Signals.h"

const int PinDIP0 = 2;
const int PinDIP1 = 3;
const int PinDIP2 = 4;
const int PinDIP3 = 5;
const int PinDIP4 = 6;
const int PinDIP5 = 7;
const int PinDIP6 = 8;
const int PinDIP7 = 9;
const int PinServo = 10;

const unsigned long ServoOffDelay   = 1000lu;
const unsigned long InitPWMInterval = 2lu;
const unsigned int InitPWMDelta     = 200;

extern volatile unsigned long signalLength;
extern volatile bool ballAvailableState;

static unsigned int minPWM      = 0;
static unsigned int maxPWM      = 0;
static byte turnMode            = 0;
static byte intervalBetweenMode = 0;
static byte intervalAfterMode   = 0;
static bool reverseMode         = false;

volatile byte dipMask           = 0;
volatile byte servoStatus       = 0;

void setup()
{
    Wire.begin(I2C_ID_SERVO);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);

    pinMode(PinDIP0, INPUT);
    pinMode(PinDIP1, INPUT);
    pinMode(PinDIP2, INPUT);
    pinMode(PinDIP3, INPUT);
    pinMode(PinDIP4, INPUT);
    pinMode(PinDIP5, INPUT);
    pinMode(PinDIP6, INPUT);
    pinMode(PinDIP7, INPUT);

    pinMode(PinServo, OUTPUT);

    setupTimer();

    readDIP();
    evaluateDIP();
    initServo();
}

void loop()
{
    if (servoStatus == SERVO_START)
    {
        readDIP();
        evaluateDIP();
        if (reverseMode == false)
        {
            servoStatus = SERVO_LOAD;
            turnServo(true);
            servoStatus = SERVO_WAIT;
            intervalBetween();
            servoStatus = SERVO_TURN;
            turnServo(false);
            intervalAfter();
            servoStatus = SERVO_DONE;
        }
        else
        {
            servoStatus = SERVO_LOAD;
            turnServo(false);
            servoStatus = SERVO_WAIT;
            intervalBetween();
            servoStatus = SERVO_TURN;
            turnServo(true);
            intervalAfter();
            servoStatus = SERVO_DONE;
        }
    }
}

void readDIP()
{
    dipMask |= digitalRead(PinDIP7);
    dipMask <<= 1;
    dipMask |= digitalRead(PinDIP6);
    dipMask <<= 1;
    dipMask |= digitalRead(PinDIP5);
    dipMask <<= 1;
    dipMask |= digitalRead(PinDIP4);
    dipMask <<= 1;
    dipMask |= digitalRead(PinDIP3);
    dipMask <<= 1;
    dipMask |= digitalRead(PinDIP2);
    dipMask <<= 1;
    dipMask |= digitalRead(PinDIP1);
    dipMask <<= 1;
    dipMask |= digitalRead(PinDIP0);
}

void evaluateDIP()
{
    turnMode = 0;
    turnMode += ((dipMask & 0x01) != 0) * (1 << 0);
    turnMode += ((dipMask & 0x02) != 0) * (1 << 1);
    turnMode += ((dipMask & 0x04) != 0) * (1 << 2);

    intervalBetweenMode = 0;
    intervalBetweenMode += ((dipMask & 0x08) != 0) * (1 << 0);
    intervalBetweenMode += ((dipMask & 0x10) != 0) * (1 << 0);

    intervalAfterMode = 0;
    intervalAfterMode += ((dipMask & 0x20) != 0) * (1 << 0);

    if ((dipMask & 0x40) == 0)
    {
        minPWM = 1000lu;
        maxPWM = 2000lu;
    }
    else
    {
        minPWM = 600lu;
        maxPWM = 2400lu;
    }

    reverseMode = ((dipMask & 0x80) != 0);
}

void initServo()
{
    if (reverseMode == false)
    {
        signalLength = maxPWM;
        while (signalLength > (maxPWM - InitPWMDelta))
        {
            signalLength--;
            delay(InitPWMInterval);
        }
        while (signalLength < maxPWM)
        {
            signalLength++;
            delay(InitPWMInterval);
        }

        // Wait to make sure servo has enough time to reach the end point.
        //
        delay(ServoOffDelay);
        
        signalLength = 0lu;
    }
    else
    {
        signalLength = minPWM;
        while (signalLength < (minPWM + InitPWMDelta))
        {
            signalLength++;
            delay(InitPWMInterval);
        }
        while (signalLength > minPWM)
        {
            signalLength--;
            delay(InitPWMInterval);
        }

        // Wait to make sure servo has enough time to reach the end point.
        //
        delay(ServoOffDelay);

        signalLength = 0lu;
    }
}

void turnServo(const bool forwards)
{
    unsigned long interval;

    switch (turnMode)
    {
        case  0: interval =  1lu; break;
        case  1: interval =  3lu; break;
        case  2: interval =  5lu; break;
        case  3: interval =  7lu; break;
        case  4: interval =  9lu; break;
        case  5: interval = 11lu; break;
        case  6: interval = 13lu; break;
        case  7: interval = 15lu; break;
        default: return;
    }

    if (forwards == true)
    {
        for (signalLength = minPWM;
             signalLength < maxPWM;
             signalLength += 10lu)
        {
            delay(interval);
        }

        // Wait to make sure servo has enough time to reach the end point.
        //
        delay(ServoOffDelay);
        
        signalLength = 0lu;
    }
    else
    {
        for (signalLength = maxPWM;
             signalLength > minPWM;
             signalLength -= 10lu)
        {
            delay(interval);
        }

        // Wait to make sure servo has enough time to reach the end point.
        //
        delay(ServoOffDelay);

        signalLength = 0lu;
    }
}

void intervalBetween()
{
    unsigned long interval;

    switch (intervalBetweenMode)
    {
        case 0: interval = 0lu; break;
        case 1: interval = 250lu; break;
        case 2: interval = 500lu; break;
        case 3: interval = 1000lu; break;
        default: return;
    }

    delay(interval);
}

void intervalAfter()
{
    unsigned long interval;

    switch (intervalAfterMode)
    {
        case 0: interval = 0lu; break;
        case 1: interval = 1000lu; break;
        default: return;
    }

    delay(interval);
}

void receiveEvent(int howMany)
{
    if (howMany == 1)
    {
        byte commandCode = Wire.read();

        if (commandCode == SERVO_START)
        {
            servoStatus = SERVO_START;
        }
        else if (commandCode == READ_DIP)
        {
            readDIP();
            Wire.write(dipMask);
        }
    }
}

void requestEvent()
{
    byte payload = servoStatus;

    if (ballAvailableState == true)
    {
        payload |= BALL_STATE;
    }

    Wire.write(payload);
}
