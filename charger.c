#include <avr/io.h>
#define F_CPU 1200000
#include <util/delay.h>
#include "config.h"

volatile int time = 0;
volatile int maxvoltage = 0;
volatile int averages[AVERAGE_LENGTH];

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

void start() {
    TURN_RED_LED_ON();
    TURN_GREEN_LED_ON();
    _delay_ms(500);
    TURN_RED_LED_OFF();
    _delay_ms(500);
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
    float total = 0;
    for(int i = 0; i < 30; i++) {
        total += read_voltage();
        _delay_ms(1000);
    }

    int average = ((total / 1023.0) * VOLTAGE_MULTIPLIER) / 30.0 + 0.5;

    // shift averages array to add a new value
    for (int i = 0; i < AVERAGE_LENGTH - 1; i++) {
        averages[i] = averages[i + 1];
    }
    averages[AVERAGE_LENGTH - 1] = average;

    float s = 0;

    for(int i = 0; i < AVERAGE_LENGTH; i++) {
        s += averages[i];
    }

    int mediumvoltage = s / (float)AVERAGE_LENGTH + 0.5;

    if(mediumvoltage > maxvoltage) {
        maxvoltage = mediumvoltage;
    }

    //If voltage is dropped then charge is finished
    if((mediumvoltage + 1) < maxvoltage) {
        return YES;
    }

    time += CHARGE_CHECK_TIME;
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
        _delay_ms(300);
    }
}

void over_heat_error() {
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

int is_over_heat() {
    int over_heat = NO;

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

void charge_impulse() {
    TURN_LOAD_ON();
    _delay_ms(IMPULSE_ON_TIME);
    TURN_LOAD_OFF();
    _delay_ms(IMPULSE_OFF_TIME);

    time += IMPULSE_ON_TIME + IMPULSE_OFF_TIME;
}

int charge_phase_1() {
    while(YES) {
        if(is_over_heat()) {
            return NO;
        }
        if(!battery_plugged()) {
            return NO;
        }

        charge_impulse();

        if(time >= PHASE_1_TIME) {
            break;
        }
    }
    return YES;
}

int charge_main_phase() {
    while(YES) {
        if(is_over_heat()) {
            return NO;
        }

        if(!battery_plugged()) {
            return NO;
        }

        if(is_charged()) {
            if(time < NO_NEED_TRICKLE_TIME) {
                charge_finished();
                return NO;
            }
            
            return YES;
        }
    }
    return YES;
}

int charge_final_phase() {
    while(YES) {
        if(is_over_heat()) {
            return NO;
        }
        if(!battery_plugged()) {
            return NO;
        }
        charge_impulse();

        if(time >= FINAL_PHASE_TIME) {
            break;
        }
    }
    return YES;
}

void charge() {
    time = 0;
    TURN_RED_LED_ON();
    TURN_GREEN_LED_OFF();
    if(charge_phase_1()) {
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

void loop() {
    TURN_LOAD_OFF();
    if(!battery_plugged()) {
        wait_for_battery();
    }
    charge();
}

int main(void) {
    _delay_ms(1000);
    setup();
    start();
    while(YES) {
        loop();
    }
    return 0;
}
