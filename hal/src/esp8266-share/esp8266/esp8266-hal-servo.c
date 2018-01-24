/*
Copyright (c) 2015 Michael C. Miller. All right reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "esp8266-hal-servo.h"
#include "esp8266-hal-gpio.h"
#include "service_debug.h"

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

#define INVALID_SERVO         255     // flag indicating an invalid servo index

const uint32_t c_CycleCompensation = 4;  // compensation us to trim adjust for digitalWrite delays

#define INVALID_PIN           63    // flag indicating never attached servo

struct ServoInfo  {
    uint8_t pin : 6;             // a pin number from 0 to 62, 63 reserved
    uint8_t isActive : 1;        // true if this channel is enabled, pin not pulsed if false
    uint8_t isDetaching : 1;     // true if this channel is being detached, maintains pulse integrity
};

struct ServoState {
    struct ServoInfo info;
    volatile uint16_t usPulse;
};

static struct ServoState s_servos[MAX_SERVOS];     // static array of servo structures

static uint8_t s_servoCount = 0;            // the total number of attached s_servos

volatile uint8_t _servoIndex;               // index into the channel data for this servo
volatile uint16_t _minUs;
volatile uint16_t _maxUs;

// inconvenience macros
#define SERVO_INDEX_TO_TIMER(servoIndex) ((ServoTimerSequence)(servoIndex / SERVOS_PER_TIMER)) // returns the timer controlling this servo
#define SERVO_INDEX(timerId, channel)  ((timerId * SERVOS_PER_TIMER) + channel)     // macro to access servo index by timer and channel

// similiar to map but will have increased accuracy that provides a more
// symetric api (call it and use result to reverse will provide the original value)
int improved_map(int value, int minIn, int maxIn, int minOut, int maxOut)
{
    const int rangeIn = maxIn - minIn;
    const int rangeOut = maxOut - minOut;
    const int deltaIn = value - minIn;
    // fixed point math constants to improve accurancy of divide and rounding
    const int fixedHalfDecimal = 1;
    const int fixedDecimal = fixedHalfDecimal * 2;

    return ((deltaIn * rangeOut * fixedDecimal) / (rangeIn) + fixedHalfDecimal) / fixedDecimal + minOut;
}


static void finISR(ServoTimerSequence timerId) ICACHE_RAM_ATTR;
static bool isTimerActive(ServoTimerSequence timerId) ICACHE_RAM_ATTR;

//------------------------------------------------------------------------------
// Interrupt handler template method that takes a class that implements
// a standard set of methods for the timer abstraction
//------------------------------------------------------------------------------
static void Servo_Handler(servo_timer_t *timer) ICACHE_RAM_ATTR;
static void Servo_Handler(servo_timer_t *timer)
{
    uint8_t servoIndex;
    // clear interrupt
    timer->ResetInterrupt();

    if (timer->isEndOfCycle()) {
        timer->StartCycle();
    }
    else {
        servoIndex = SERVO_INDEX(timer->timerId(), timer->getCurrentChannel());
        if (servoIndex < s_servoCount && s_servos[servoIndex].info.isActive) {
            // pulse this channel low if activated
            __digitalWrite(s_servos[servoIndex].info.pin, LOW);

            if (s_servos[servoIndex].info.isDetaching) {
                s_servos[servoIndex].info.isActive = false;
                s_servos[servoIndex].info.isDetaching = false;
            }
        }
        timer->nextChannel();
    }

    servoIndex = SERVO_INDEX(timer->timerId(), timer->getCurrentChannel());

    if (servoIndex < s_servoCount &&
        timer->getCurrentChannel() < SERVOS_PER_TIMER) {
        timer->SetPulseCompare(timer->usToTicks(s_servos[servoIndex].usPulse) - c_CycleCompensation);

        if (s_servos[servoIndex].info.isActive) {
            if (s_servos[servoIndex].info.isDetaching) {
                // it was active, reset state and leave low
                s_servos[servoIndex].info.isActive = false;
                s_servos[servoIndex].info.isDetaching = false;
            }
            else {
                // its an active channel so pulse it high
                __digitalWrite(s_servos[servoIndex].info.pin, HIGH);
            }
        }
    }
    else {
        if (!isTimerActive(timer->timerId())) {
            // no active running channels on this timer, stop the ISR
            finISR(timer->timerId());
        }
        else {
            // finished all channels so wait for the refresh period to expire before starting over
            // allow a few ticks to ensure the next match is not missed
            uint32_t refreshCompare = timer->usToTicks(REFRESH_INTERVAL);
            if ((timer->GetCycleCount() + c_CycleCompensation * 2) < refreshCompare) {
                timer->SetCycleCompare(refreshCompare - c_CycleCompensation);
            }
            else {
                // at least REFRESH_INTERVAL has elapsed
                timer->SetCycleCompare(timer->GetCycleCount() + c_CycleCompensation * 2);
            }
        }

        timer->setEndOfCycle();
    }
}

static void handler0() ICACHE_RAM_ATTR;
static void handler0()
{
#if !defined (SERVO_EXCLUDE_TIMER0)
    Servo_Handler(&s_servoTimer0);
#endif
}

static void handler1() ICACHE_RAM_ATTR;
static void handler1()
{
#if !defined (SERVO_EXCLUDE_TIMER1)
    Servo_Handler(&s_servoTimer1);
#endif
}

static void initISR(ServoTimerSequence timerId)
{
#if !defined (SERVO_EXCLUDE_TIMER0)
    if (timerId == ServoTimerSequence_Timer0)
        s_servoTimer0.InitInterrupt(&handler0);
#endif
#if !defined (SERVO_EXCLUDE_TIMER1)
    if (timerId == ServoTimerSequence_Timer1)
        s_servoTimer1.InitInterrupt(&handler1);
#endif
}

static void finISR(ServoTimerSequence timerId)
{
#if !defined (SERVO_EXCLUDE_TIMER0)
    if (timerId == ServoTimerSequence_Timer0)
        s_servoTimer0.StopInterrupt();
#endif
#if !defined (SERVO_EXCLUDE_TIMER1)
    if (timerId == ServoTimerSequence_Timer1)
        s_servoTimer1.StopInterrupt();
#endif
}

// returns true if any servo is active on this timer
static bool isTimerActive(ServoTimerSequence timerId)
{
    for (uint8_t channel = 0; channel < SERVOS_PER_TIMER; channel++) {
        if (s_servos[SERVO_INDEX(timerId, channel)].info.isActive) {
            return true;
        }
    }
    return false;
}

static uint8_t isPinActive(int8_t pin)
{
    uint8_t index;
    for(index = 0;index <= _servoIndex; index++)
    {
        if(s_servos[index].info.pin == pin)
        {
            return index;
        }
    }
    return 0;
}

void ESP8266ServoInit(void)
{
    if (s_servoCount < MAX_SERVOS) {
        // assign a servo index to this instance
        _servoIndex = s_servoCount++;
        // store default values
        s_servos[_servoIndex].usPulse = DEFAULT_PULSE_WIDTH;

        // set default _minUs and _maxUs incase write() is called before attach()
        _minUs = MIN_PULSE_WIDTH;
        _maxUs = MAX_PULSE_WIDTH;

        s_servos[_servoIndex].info.isActive = false;
        s_servos[_servoIndex].info.isDetaching = false;
        s_servos[_servoIndex].info.pin = INVALID_PIN;
    }
    else {
        _servoIndex = INVALID_SERVO;  // too many servos
    }
}

uint8_t ESP8266ServoAttachHandle(int pin, int minUs, int maxUs)
{
    ServoTimerSequence timerId;

    if (_servoIndex < MAX_SERVOS) {
        if (s_servos[_servoIndex].info.pin == INVALID_PIN) {
            __pinMode(pin, ESP8266_OUTPUT);       // set servo pin to output
            __digitalWrite(pin, LOW);
            s_servos[_servoIndex].info.pin = pin;
        }

        // keep the min and max within 200-3000 us, these are extreme
        // ranges and should support extreme servos while maintaining
        // reasonable ranges
        _maxUs = max(250, min(3000, maxUs));
        _minUs = max(200, min(_maxUs, minUs));

        // initialize the timerId if it has not already been initialized
        timerId = SERVO_INDEX_TO_TIMER(_servoIndex);
        if (!isTimerActive(timerId)) {
            initISR(timerId);
        }

        s_servos[_servoIndex].info.isDetaching = false;
        s_servos[_servoIndex].info.isActive = true;  // this must be set after the check for isTimerActive
    }
    return _servoIndex;
}

static bool servoTimer0En = false;
static bool servoTimer1En = false;

uint8_t ESP8266ServoAttach(int pin)
{
#if !defined (SERVO_EXCLUDE_TIMER0)
    if(!servoTimer0En)
    {
        ESP8266ServoTimer0();
        servoTimer0En = true;
    }
#endif

#if !defined (SERVO_EXCLUDE_TIMER1)
    if(!servoTimer1En)
    {
        ESP8266ServoTimer1();
        servoTimer1En = true;
    }
#endif

    ESP8266ServoInit();
    return ESP8266ServoAttachHandle(pin, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
}

void ESP8266ServoDetach(int8_t pin)
{
    //ServoTimerSequence timerId;
    uint8_t index;
    index = isPinActive(pin);
    if (s_servos[index].info.isActive) {
        s_servos[index].info.isDetaching = true;
    }
}

void ESP8266ServoWriteMicroseconds(int8_t pin, int value)
{
    // ensure channel is valid
    if ((_servoIndex < MAX_SERVOS)) {
        // ensure pulse width is valid
        value = constrain(value, _minUs, _maxUs);

        /* s_servos[_servoIndex].usPulse = value; */
        s_servos[isPinActive(pin)].usPulse = value;
    }
}

void ESP8266ServoWrite(int8_t pin, int value)
{
    // treat values less than 544 as angles in degrees (valid values in microseconds are handled as microseconds)
    if (value < MIN_PULSE_WIDTH) {
        // assumed to be 0-180 degrees servo
        value = constrain(value, 0, 180);
        // writeMicroseconds will contrain the calculated value for us
        // for any user defined min and max, but we must use default min max
        value = improved_map(value, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
    }
    ESP8266ServoWriteMicroseconds(pin,value);
}

int ESP8266ServoReadMicroseconds(int8_t pin)
{
    unsigned int pulsewidth;
    if (_servoIndex != INVALID_SERVO) {
        /* pulsewidth = s_servos[_servoIndex].usPulse; */
        pulsewidth = s_servos[isPinActive(pin)].usPulse;
    }
    else {
        pulsewidth = 0;
    }

    return pulsewidth;
}

int ESP8266ServoRead(int8_t pin) // return the value as degrees
{
    // read returns the angle for an assumed 0-180, so we calculate using
    // the normal min/max constants and not user defined ones
    return improved_map(ESP8266ServoReadMicroseconds(pin), MIN_PULSE_WIDTH, MAX_PULSE_WIDTH, 0, 180);
}

bool ESP8266ServoAttached(int8_t pin)
{
    return s_servos[isPinActive(pin)].info.isActive;
}
