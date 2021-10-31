/**
 * @file clockMaker.h
 * @author Léo Chauvin Damien Frissant
 * @brief the header file of clockMaker.c
 * @version 0.1
 * @date 2021-04-27
 * 
 * @copyright © 2021, Turfly company.
 * 
 */

#ifndef CLOCKMAKER_H
#define CLOCKMAKER_H

#include "./../common.h"

/**
 * Describes the format of the time sent to the Raspberry
*/
typedef struct{
    char data[MAX_BUFFER_SIZE/2];
} DateTime;

/**
 * @brief Set the time, UTC+1
 * 
 * @param time is a string of 19 characters :YYYY-MM-DD HH:MM:SS
 */
extern void ClockMaker_setCurrentDateTime(DateTime dateTime);

/**
 * @brief Set the Default Date and the Default Time objects
 * 
 */
extern void ClockMaker_setDefaultDateTime(void);

/**
 * @brief get the current DateTime as YYYY-MM-DD HH:MM:SS
 * 
 */
extern DateTime ClockMaker_getCurrentDateTime(void);
#endif /* CLOCKMAKER_H */
