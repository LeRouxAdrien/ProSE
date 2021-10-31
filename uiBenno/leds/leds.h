/**
 * @file leds.h
 * @author Adrien LE ROUX
 * @brief 
 * @version 0.1
 * @date 2021-06-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */
typedef struct{
    char *gpio;
    struct gpiod_line *outputLine;
}Led;

/**
 * @brief intialize leds ports
 * 
 */
extern void Leds_init();

/**
 * @brief switch on the led.
 * 
 */
extern void Leds_turnOnLed (Led led);

/**
 * @brief switch off the led.
 * 
 */

extern void Leds_turnOffLed (Led led);

/**
 * @brief free the leds ports
 * 
 */
extern void Leds_free ();

/**
 * @brief Allow Speaker to get the leds.
 * 
 */
extern Led *Leds_getLeds();
