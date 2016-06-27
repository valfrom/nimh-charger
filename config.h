#define PHASE_1_TIME 600

#define IMPULSE_ON_TIME 750
#define IMPULSE_OFF_TIME 250

#define NO_NEED_TRICKLE_TIME 900

#define FINAL_PHASE_TIME 900

#define CHARGE_CHECK_TIME 30

#define YES 1
#define NO 0

#define BATTERY_PLUS_PIN PB3
#define THERMO_PIN PB4

#define LOAD_PIN PB2
#define RED_LED_PIN PB0
#define GREEN_LED_PIN PB1

#define TURN_RED_LED_ON() PORTB |= (1<<RED_LED_PIN)
#define TURN_RED_LED_OFF() PORTB &= ~(1<<RED_LED_PIN)

#define TURN_GREEN_LED_ON() PORTB |= (1<<GREEN_LED_PIN)
#define TURN_GREEN_LED_OFF() PORTB &= ~(1<<GREEN_LED_PIN)

#define TURN_LOAD_ON() PORTB |= (1<<LOAD_PIN)
#define TURN_LOAD_OFF() PORTB &= ~(1<<LOAD_PIN)

// Multiplier to convert from analog read to millivolts
#define VOLTAGE_MULTIPLIER 14.0

#define AVERAGE_LENGTH 7