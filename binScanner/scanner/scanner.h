/**
 * @file scanner.h
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

#ifndef SCANNER_H
#define SCANNER_H

#include <stdint.h>   // uint8_t, uint64_t
#include <stdio.h>    // printf
#include <sys/stat.h> /* Pour les constantes « mode » */
#include <mqueue.h>
#include <pthread.h>

#include "../../watchdog/watchdog.h"
#include "../adminScanner/adminScanner.h"

typedef struct
{
    char *idBin; // 8 caractères au format hexadécimal
} IdBin;

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                  FUNCTIONS PROTOTYPES                               ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Search the RFID tag, and when one is detected, get the tag and wait
 * for X seconds before searching a new one.
 * 
 */
extern IdBin Scanner_askIdBin(void);

/**
 * @brief Set the global variable to the value of the last tag detected.
 * 
 * @param IdBenne The ID of the tag scanned.
 * 
 */
extern void Scanner_setIdBin(IdBin idBenne);

/**
 * @brief Close all ports used by RFID sensor and close config file.
 * 
 */
extern void Scanner_stop(void);

/**
 * @brief Initialise ports and read config file for RC522 RFID sensor
 * 
 */
extern void Scanner_initialise(void);
#endif