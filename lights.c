#include <avr/io.h>
#include "lights.h"

void lights_init(void) {
    // Set the data direction register for the lights port to output
    DDRA |= (1 << PA0); // Assuming light is connected to PA0
    DDRA |= (1 << PA1); // Assuming light is connected to PA1

    PORTA &= ~(1 << PA0); // Turn off light connected to PA0
    PORTA &= ~(1 << PA1); // Turn off light connected to PA1

}

void lights_set(light_state_t state) {
    switch (state) {
        case ALL_OFF:
            PORTA &= ~(1 << PA0); // Turn off light connected to PA0
            PORTA &= ~(1 << PA1); // Turn off light connected to PA1
            break;
        case LEFT_ON:
            PORTA |= (1 << PA0); // Turn on light connected to PA0
            PORTA &= ~(1 << PA1); // Turn off light connected to PA1
            break;
        case RIGHT_ON:
            PORTA &= ~(1 << PA0); // Turn off light connected to PA0
            PORTA |= (1 << PA1); // Turn on light connected to PA1
            break;
        case HAZARD_ON:
            PORTA |= (1 << PA0); // Turn on light connected to PA0
            PORTA |= (1 << PA1); // Turn on light connected to PA1
            break;
        default:
            // Handle invalid state if necessary
            break;
    }
}
    
