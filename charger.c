#include <avr/io.h>
#define F_CPU 1200000
#include <util/delay.h>
#include "config.h"

unsigned int time = 0;
unsigned int maxvoltage = 0;
unsigned int averages[AVERAGE_LENGTH];
unsigned int total = 0;
unsigned int numReadings = 0;

void setup() {
    // Set output pins
    DDRB |= (1<<RED_LED_PIN);
    DDRB |= (1<<GREEN_LED_PIN);
    DDRB |= (1<<LOAD_PIN);

    // Set input pins
    DDRB &= ~(1<<BATTERY_PLUS_PIN);
    DDRB &= ~(1<<THERMO_PIN);

    // Set the prescaler to clock/128 & enable ADC
    ADCSRA |= (1 << ADPS1) | (1 << ADPS0) | (1 << ADEN);
}

int analog_read() {
    // Start the conversion
    ADCSRA |= (1 << ADSC);

    // Wait for it to finish
    while (ADCSRA & (1 << ADSC)) {
        ;
    }

    return ADC;
}

int read_voltage() {
    // Set the ADC input to PB4/ADC2
    ADMUX |= (1 << MUX1);
    ADMUX |= (0 << MUX0);

    int val = analog_read();

    return val;
}

int is_charged() {
    int voltage = read_voltage();
    total += voltage;

    numReadings ++;

    if(numReadings < READINGS_COUNT) {
        return NO;
    }

    numReadings = 0;

    unsigned int average = (total / READINGS_COUNT) * VOLTAGE_MULTIPLIER / 1023;

    // Shift averages array to add a new value
    for (int i = 0; i < AVERAGE_LENGTH - 1; i++) {
        averages[i] = averages[i + 1];
    }
    averages[AVERAGE_LENGTH - 1] = average;

    unsigned int s = 0;

    for(int i = 0; i < AVERAGE_LENGTH; i++) {
        s += averages[i];
    }

    int mediumvoltage = s / AVERAGE_LENGTH;

    if(mediumvoltage > maxvoltage) {
        maxvoltage = mediumvoltage;
    }

    // If voltage is dropped then charge is finished
    if((mediumvoltage + 100) < maxvoltage) {
        return YES;
    }
    
    return NO;
}

int battery_plugged() {
    int val = read_voltage();
    if(val > 70) {
        return YES;
    }
    return NO;
}

void wait_for_battery() {
    while(YES) {
        if(battery_plugged()) {
            break;
        }
        TURN_GREEN_LED_ON();
        _delay_ms(400);
        TURN_GREEN_LED_OFF();
        _delay_ms(400);
    }
}

void charge_finished() {
    TURN_LOAD_OFF();
    TURN_RED_LED_OFF();
    TURN_GREEN_LED_ON();
    while(YES) {
        if(!battery_plugged()) {
            break;
        }
        _delay_ms(300);
    }
}

#if OVERHEAT_SUPPORT
int is_over_heat() {
    // Set the ADC input to PB3/ADC3
    ADMUX |= (1 << MUX1);
    ADMUX |= (1 << MUX0);

    int val = analog_read();

    if(val > 512) {
        over_heat_error();
        return YES;
    }
    return NO;
}

void over_heat_error() {
    TURN_GREEN_LED_OFF();
    TURN_LOAD_OFF();
    while(YES) {
        if(!battery_plugged()) {            
            break;
        }
        TURN_RED_LED_ON();
        _delay_ms(400);
        TURN_RED_LED_OFF();
        _delay_ms(400);
    }
}
#endif

void maximum_time_charge_error() {    
    TURN_LOAD_OFF();
    while(YES) {
        if(!battery_plugged()) {
            break;
        }
        TURN_GREEN_LED_ON();
        TURN_RED_LED_ON();
        _delay_ms(400);
        TURN_GREEN_LED_OFF();
        TURN_RED_LED_OFF();
        _delay_ms(400);
    }
}

int charge_phase_1() {
    while(YES) {
        #if OVERHEAT_SUPPORT
        if(is_over_heat()) {
            return NO;
        }
        #endif
        
        if(!battery_plugged()) {
            return NO;
        }

        TURN_LOAD_ON();
        TURN_RED_LED_ON();
        _delay_ms(IMPULSE_ON_TIME);
        //Check voltage, but ignore readings
        is_charged();
        TURN_LOAD_OFF();
        TURN_RED_LED_OFF();
        _delay_ms(IMPULSE_OFF_TIME);

        time++;

        if(time >= PHASE_1_TIME) {
            break;
        }
    }
    return YES;
}

int charge_main_phase() {
    TURN_LOAD_ON();
    TURN_RED_LED_ON();
    while(YES) {
        #if OVERHEAT_SUPPORT
        if(is_over_heat()) {
            return NO;
        }
        #endif

        if(!battery_plugged()) {
            return NO;
        }

        _delay_ms(FULL_IMPULSE_TIME);
        int charged = is_charged();

        time ++;

        if(charged) {            
            return YES;
        }

        if(time > MAXIMUM_CHARGE_TIME) {
            maximum_time_charge_error();
            return NO;
        }
    }
    return YES;
}

int charge_final_phase() {
    while(YES) {
        #if OVERHEAT_SUPPORT
        if(is_over_heat()) {
            return NO;
        }
        #endif

        if(!battery_plugged()) {
            return NO;
        }

        TURN_LOAD_ON();
        TURN_RED_LED_ON();
        _delay_ms(IMPULSE_ON_TIME);
        TURN_LOAD_OFF();
        TURN_RED_LED_OFF();
        _delay_ms(IMPULSE_OFF_TIME);

        time ++;

        if(time >= FINAL_PHASE_TIME) {
            break;
        }
    }
    return YES;
}

void charge() {    
    TURN_RED_LED_OFF();
    TURN_GREEN_LED_OFF();
    if(!charge_phase_1()) {
        return;
    }

    if(!charge_main_phase()) {
        return;
    }

    if(!charge_final_phase()) {
        return;
    }
    charge_finished();
}

void reset() {
    numReadings = 0;
    total = 0;
    time = 0;
    maxvoltage = 0;
    
    for(int i = 0; i < AVERAGE_LENGTH; i++) {
        averages[AVERAGE_LENGTH] = 0;
    }
}

void loop() {
    TURN_LOAD_OFF();
    TURN_RED_LED_OFF();
    TURN_GREEN_LED_OFF();
    if(!battery_plugged()) {
        wait_for_battery();
    }
    reset();
    charge();
}

int main(void) {
    _delay_ms(1000);
    setup();
    while(YES) {
        loop();
    }
    return 0;
}
