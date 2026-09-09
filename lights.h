#idenf LIGHTS_H
#define LIGHTS_H

typedef enum {
    ALL_OFF, 
    LEFT_ON,
    RIGHT_ON,
    HAZARD_ON,
} LightState_t;

void lights_init(void);
void lights_set(LightState_t state);
#endif // LIGHTS_H