/**
 * @file postmanBenno.h
 * @author Léo Chauvin
 * @brief the header file of postmanBenno.c
 * @version 2.0
 * @date 2021-04-27
 * @copyright © 2021, Turfly company.
 * 
 */

#ifndef POSTMAN_BENNO_H
#define POSTMAN_BENNO_H
#include <stdio.h>
#include <stdint.h>

/**
 * @brief Opens the letterbox
 * 
 */
extern void PostmanBenno_new();

/**
 * @brief Initialises the listening and data sockets
 * 
 */
extern void PostmanBenno_start(void);

/**
 * @brief Creates the writing pthread
 * 
 */
extern void PostmanBenno_createWritingPthread(void);

/**
 * @brief Creates the reading pthread
 * 
 */
extern void PostmanBenno_createReadingPthread(void);

/**
 * @brief Closes and destroy the letterbox
 * 
 */
extern void PostmanBenno_free(void);

/**
 * @brief Closes the listening and data sockets
 * 
 */
extern void PostmanBenno_stop(void);

/**
 * @brief Write data to Visio
 * 
 */
extern void PostmanBenno_sendMsg(char * msg);

#endif /* POSTMAN_BENNO_H */