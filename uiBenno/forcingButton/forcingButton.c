/**
 * @file forcingButton.c
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
#include <pthread.h>
#include "forcingButton.h"
#include "../../utils.h"
#include "../speaker/speaker.h"
static char forcingButton[1] = "6"; //link GPIO_6 and GND together
static char outputF[15] = "/dev/gpiochip0";
static char prog[21] = "./benno_raspberry.out";

static int inputOffset;
static struct gpiod_chip *inputChip;
static struct gpiod_line *inputLine;
static int stateButton;
static pthread_t thread;
static bool read1 = 1;

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                  FUNCTIONS PROTOTYPES                               ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief run create a thread which permanently read the button state only if the button has a state changement (event phenomenon)
 * 
 * @param aParam 
 * @return void* 
 */
static void *run(void *aParam);

/**
 * @brief evaluate if the buton has been pushing or not.
 * 
 * @param aParam 
 * @return void* 
 */
static void evaluatePushing(int buttonState);

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                   STATIC FUNCTIONS                                  ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

static void *run(void *aParam)
{
    while (read1 == 1)
    {
        stateButton = ForcingButton_getStateReadButton();
        evaluatePushing(stateButton);
    }
    return NULL;
}

static void evaluatePushing(int buttonState)
{
    if (buttonState == 1)
    {
        Speaker_forceLiftBin();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                   EXTERN FUNCTIONS                                  ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

extern void ForcingButton_init()
{
    if (sscanf(forcingButton, "%d", &inputOffset) != 1) // convert the gpio's number of the orange led string into an integer
    {
        fprintf(stderr, "%s: invalid <input-offset> value.\n", forcingButton);
        exit(EXIT_FAILURE);
    }
    inputChip = (struct gpiod_chip *)gpiod_chip_open_lookup(outputF); // Allow the gpiochip0's controller to be assigned and used to take input pins over
    if (inputChip == NULL)
    {
        perror(outputF);
        exit(EXIT_FAILURE);
    }
    inputLine = gpiod_chip_get_line(inputChip, inputOffset); // Assign the gpio to the controller
    if (inputLine == NULL)
    {
        perror(forcingButton);
        exit(EXIT_FAILURE);
    }
    if (gpiod_line_request_both_edges_events(inputLine, prog) < 0) // Define and initialize the following gpio in input and activate the both event edges events
    {
        perror(forcingButton);
        exit(EXIT_FAILURE);
    }
}

extern void ForcingButton_free()
{
    gpiod_line_release(inputLine);
}

extern int ForcingButton_getStateReadButton()
{
    int state;
    struct gpiod_line_event event;
    gpiod_line_event_wait(inputLine, NULL);            // Assign a wait state to the lifting button
    if (gpiod_line_event_read(inputLine, &event) == 0) // Read the pin only if there is a state change
    {
        if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) // Pull down logic : If the state change come from 0 to 1
        {
            state = 1;
        }
        else
        {
            state = 0;
        }
    }
    return state;
}

extern void ForcingButton_startPolling()
{
    int8_t check;
    check = pthread_create(&thread, NULL, &run, NULL);
    if (check != 0)
    {
        PERROR("Error when create the Forcing Button thread");
    }
}


extern void ForcingButton_stopPolling()
{
    read1 = 0;
    pthread_cancel(thread);
}
