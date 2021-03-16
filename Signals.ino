#include <avr/io.h>
#include <avr/interrupt.h>

#include "Signals.h"

volatile unsigned long signalLength = 0lu;

void setupTimer()
{
    // Stop timer and reset prescaler.
    //
    GTCCR |= (1 << TSM) | (1 << PSRASY);

    // Select asynchron mode.
    //
    ASSR |= (1 << AS2);

    // Set CTC mode.
    //
    TCCR2A = (1 << WGM21);

    // Set prescaler 256.
    //
    TCCR2B |= (1 << CS22) | (1 << CS21);

    // Right value for timer based on selected prescaler.
    // A value of 124 will let timer be triggered every 2 milliseconds.
    //
    OCR2A = 124 - 1;

    // Enable compare interrupt.
    //
    TIMSK2 |= (1 << OCIE2A);

    // Start timer.
    //
    GTCCR &= ~(1 << TSM);

    // Activate global interrupts.
    //
    sei();
}

// Counter to be used in interrupt handler to count the cycles to skip.
// Due to limitations of the timer with prescaler 256, only each 10th cycle should be handled,
// because only each 10th will be triggered every 20 milliseconds.
//
volatile byte skipCycleCounter = 0;

ISR(TIMER2_COMPA_vect)
{
    TCCR2B = TCCR2B;

    skipCycleCounter++;
    if (skipCycleCounter == 10)
    {
        skipCycleCounter = 0;

        if (signalLength > 0)
        {
            PORTB |= (1 << PORTB2);
            delayMicroseconds(signalLength);
            PORTB &= ~(1 << PORTB2);
        }
    }

    while (ASSR & ((1 << TCN2UB) | (1 << OCR2AUB) | (1 << OCR2BUB) | (1 << TCR2AUB) | (1 << TCR2BUB))) { }
}
