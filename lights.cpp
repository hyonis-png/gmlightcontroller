#include <avr/io.h>
#include "lights.h"

void lights_init(void)
{
    // Start both lights LOW.
    PORTJ &= ~(1 << PJ1);
    PORTJ &= ~(1 << PJ0);

    // D14 and D15: outputs.
    DDRJ |= (1 << PJ1);
    DDRJ |= (1 << PJ0);
}

void lights_set(LightState_t state)
{
    switch (c)
    {
        case ALL_OFF:
            PORTJ &= ~(1 << PJ1);
            PORTJ &= ~(1 << PJ0);
            break;

        case LEFT_ON:
            PORTJ |= (1 << PJ1);
            PORTJ &= ~(1 << PJ0);
            break;

        case RIGHT_ON:
            PORTJ &= ~(1 << PJ1);
            PORTJ |= (1 << PJ0);
            break;

        case HAZARD_ON:
            PORTJ |= (1 << PJ1);
            PORTJ |= (1 << PJ0);
            break;

        default:
            break;
    }
}