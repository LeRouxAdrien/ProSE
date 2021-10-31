/**
 * @file dispatcherBenno.h
 * @author Léo Chauvin
 * @brief the header file of dispatcherBenno.c
 * @version 2.0
 * @date 2021-04-27
 * @copyright © 2021, Turfly company.
 * 
 */

#ifndef DISPATCHER_BENNO_H
#define DISPATCHER_BENNO_H
#include <stdio.h>
#include <stdint.h>

/**
 * @brief Call the appropriate function according to the buffer received
 * 
 * @param receiveBuffer The buffer received from postmanBenno
 */
extern void DispatcherBenno_setMsg(char *receiveBuffer);

#endif /* DISPATCHER_BENNO_H */