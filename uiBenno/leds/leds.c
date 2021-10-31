/**
 * @file leds.c
 * @author Adrien LE ROUX
 * @brief 
 * @version 0.1
 * @date 2021-06-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <gpiod.h>
#include <stdio.h>
#include <stdbool.h> // bool
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "leds.h"

// char *oLed = "24"; //link GPIO_24 and GND together
// char *rLed = "26"; //link GPIO_26 and GND together
// char *gLed = "23"; //link GPIO_23 and GND together
// char *wLed = "22"; //link GPIO_22 and GND together
static char outputF[15] = "/dev/gpiochip0";
static char prog[21] = "./benno_raspberry.out";
static Led leds[4];
static int outputOffset[4];
static struct gpiod_chip *outputChip;
static char *cled[4] = {"24", "25", "26", "27"};
extern void Leds_init()
{
    for (int i = 0; i <= 3; i++)
    {
        if (sscanf(cled[i], "%d", &outputOffset[i]) != 1) // convert the gpio's number of the orange led string into an integer
        {
            fprintf(stderr, "%s: invalid <input-offset> value.\n", cled[i]);
            exit(EXIT_FAILURE);
        }
    }
    outputChip = (struct gpiod_chip *)gpiod_chip_open_lookup(outputF); // Allow the gpiochip0's controller to be assigned and used to control output pins
    if (outputChip == NULL)
    {
        perror(outputF);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i <= 3; i++)
    {
        leds[i].gpio = cled[i];
        leds[i].outputLine = gpiod_chip_get_line(outputChip, outputOffset[i]); // Assign the gpio to the controller
        if (leds[i].outputLine == NULL)
        {
            perror(cled[i]);
            exit(EXIT_FAILURE);
        }
        if (gpiod_line_request_output(leds[i].outputLine, prog, 0) < 0) // Define and initialize the following gpio in output
        {
            perror(cled[i]);
            exit(EXIT_FAILURE);
        }
        Leds_turnOffLed(leds[i]); // secure, to enforce the turning off process
    }
}

extern Led *Leds_getLeds()
{
    return leds;
}

extern void Leds_turnOnLed(Led led)
{
    gpiod_line_set_value(led.outputLine, 1); // turn on the green led
}

extern void Leds_turnOffLed(Led led)
{
    gpiod_line_set_value(led.outputLine, 0); // turn on the green led
}

extern void Leds_free()
{
    for (int i = 0; i <= 3; i++)
    {
        Leds_turnOffLed(leds[i]);
        gpiod_line_release(leds[i].outputLine);
    }
}
