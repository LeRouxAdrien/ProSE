/**
 * @file scales.c
 * @author Léo Chauvin
 * @brief scales get the weight of the bins.
 * @version 2.0
 * @date 2021-06-07
 * 
 * @copyright © 2021, Turfly company.
 * 
 */

#include <stdlib.h>
#include <stdbool.h>
#include <gpiod.h>
#include "scales.h"
#include "./../tourManager/tourManager.h"
#include "./../utils.h"
#include "./../common.h"

typedef struct{
    char place[2];                  /**<GPIO of the pin*/
    int inputOffset;                /**<Offset of the pin*/
    struct gpiod_line *inputLine;   /**<Line of the pin*/
    int lastValue;                  /**<Previous state of the pin*/
    int newValue;                   /**<Current state of the pin*/
} Pin;

#define MAX_BINWEIGHT 500

Pin pinClk = {
    .place = "2",
    .inputOffset = 0,
    .lastValue = 1, // 0 = between 2 notches, 1 = on a notch
    .newValue = 1,
};

Pin pinDt = {
    .place = "3",
    .inputOffset = 0,
    .lastValue = 1,
    .newValue = 1,
};

Pin pinSw = {
    .place = "4",
    .inputOffset = 0,
    .lastValue = 1, // 0 = button pressed, 1 = button released
    .newValue = 1,
};

static struct gpiod_chip *inputChip;
static char gpiodChipDescriptor[15] = "/dev/gpiochip0";
static char consumerName[21] = "./benno_raspberry.out";

#ifndef _WRAP_GETWEIGHT
/**
 * @brief Get the Weight of the bin
 * 
 * @return Weight The weight of the bin
 */
static Weight getWeight();
#else
Weight getWeight(void);
#endif

extern void Scales_new()
{
    /* INITIALISATION OF THE INPUT CHIP */
    inputChip = gpiod_chip_open_lookup(gpiodChipDescriptor);
	if (inputChip == NULL) {
		perror(gpiodChipDescriptor);
		exit(EXIT_FAILURE);
	}

    /* INITIALISATION OF THE OFFSET */
    if (sscanf(pinClk.place, "%d", &pinClk.inputOffset) != 1){
        PERROR("%s: invalid <input-offset> value.\n", pinClk.place);
        exit(EXIT_FAILURE);
    }

    if (sscanf(pinDt.place, "%d", &pinDt.inputOffset) != 1){
        PERROR("%s: invalid <input-offset> value.\n", pinDt.place);
        exit(EXIT_FAILURE);
    }

    if (sscanf(pinSw.place, "%d", &pinSw.inputOffset) != 1){
        PERROR("%s: invalid <input-offset> value.\n", pinSw.place);
        exit(EXIT_FAILURE);
    }

    /* INITIALISATION OF THE INPUT LINE */
    pinClk.inputLine = gpiod_chip_get_line(inputChip, pinClk.inputOffset);
    if (pinClk.inputLine == NULL) {
		perror(pinClk.place);
		exit(EXIT_FAILURE);
	}

    pinDt.inputLine = gpiod_chip_get_line(inputChip, pinDt.inputOffset);
    if (pinDt.inputLine == NULL) {
		perror(pinDt.place);
		exit(EXIT_FAILURE);
	}

    pinSw.inputLine = gpiod_chip_get_line(inputChip, pinSw.inputOffset);
    if (pinSw.inputLine == NULL) {
		perror(pinSw.place);
		exit(EXIT_FAILURE);
	}

    /* RESERVE ACCESS TO THE INPUT LINE */
    if (gpiod_line_request_input(pinClk.inputLine, consumerName) < 0) {
		perror(pinClk.place);
		exit(EXIT_FAILURE);
	}

    if (gpiod_line_request_input(pinDt.inputLine, consumerName) < 0) {
		perror(pinDt.place);
		exit(EXIT_FAILURE);
	}

    if (gpiod_line_request_input(pinSw.inputLine, consumerName) < 0) {
		perror(pinSw.place);
		exit(EXIT_FAILURE);
	}
    
    TRACE("Initialisation de Scales terminée\n");
}

extern void Scales_free()
{
    gpiod_line_release(pinClk.inputLine);
    gpiod_line_release(pinDt.inputLine);
    gpiod_line_release(pinSw.inputLine);
}

extern void Scales_askWeight()
{
    Weight weight = getWeight();
    TRACE("Weight sent : %d\n", weight);
    TourManager_setWeight(weight);
}

#ifndef _WRAP_GETWEIGHT
static Weight getWeight()
{
    Weight weight = 0;
    bool validate = false;

    while(!validate)
    {
        pinSw.newValue = gpiod_line_get_value(pinSw.inputLine);

        if (pinSw.lastValue && !pinSw.newValue) {      // To validate the weight
            validate = true;
            TRACE("Weight validated\n");
        }

        pinClk.newValue = gpiod_line_get_value(pinClk.inputLine);
            
        if (!pinClk.lastValue && pinClk.newValue) {
            pinDt.newValue = gpiod_line_get_value(pinDt.inputLine);
            if (pinDt.newValue == 1) {
                weight++;
                if (weight > MAX_BINWEIGHT) {
                    weight = MAX_BINWEIGHT;
                }
                TRACE("Weight : %d kg\n", weight);
            }
        }
        pinSw.lastValue = pinSw.newValue;
        pinClk.lastValue = pinClk.newValue;
    }

    return weight;
}
#endif
