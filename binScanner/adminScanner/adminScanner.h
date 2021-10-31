/**
 * @file adminScanner.h
 *
 * @author Antoine Rivière
 * 
 * @brief the header file of adminScanner.c
 * 
 * @version 2.0
 * 
 * @date 2021-04-27
 * 
 * @copyright © 2021, Turfly company.
 * 
 */

#ifndef ADMIN_SCANNER_H
#define ADMIN_SCANNER_H

#include <stdint.h>   // uint8_t, uint64_t
#include <stdio.h>    // printf
#include <sys/stat.h> /* Pour les constantes « mode » */
#include <mqueue.h>
#include <pthread.h>

#include "../scanner/scanner.h"

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                  TYPEDEF & VARIABLES                                ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

// Taking variables from rfidfct.c
extern uint8_t use_gpio;   // was valid GPIO for reset ?
extern uint8_t gpio;       // GPIO for hard-reset
extern uint32_t spi_speed; // Overruled by config file value

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                  FUNCTIONS PROTOTYPES                               ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Functions that create the message queue with different parameters.
 * 
 */
extern void AdminScanner_new(void);

/**
 * @brief Send the message from message queue that will be an Event set in several functions.
 * 
 */
extern void AdminScanner_startBinScanner(void);
/**
 * @brief Send the message from message queue that will be an Event set in several functions.
 * 
 */
extern void AdminScanner_stopBinScanner(void);

/**
 * @brief Close the letterbox and destroy it
 * 
 */
extern void AdminScanner_free(void);

#endif