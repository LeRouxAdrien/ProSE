/**
 * @file clockMaker.c
 * @author Léo Chauvin Damien Frissant
 * @brief
 * @version 2.0
 * @date 2021-04-27
 * 
 * @copyright © 2021, Turfly company.
 * 
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <locale.h>
#include <pthread.h>
#include <errno.h>
#include "clockMaker.h"
#include "./../utils.h"

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                  TYPEDEF & VARIABLES                                ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

static DateTime myCurrentDateTime;

/**
 * @brief mutex to protect the read/write of the DateTime
 * 
 */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/////////////////////////////////////////////////////////////////////////////////////////////
////                                                                                     ////
////                                   EXTERN FUNCTIONS                                  ////
////                                                                                     ////
/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _WRAP_SETDATI
extern void ClockMaker_setCurrentDateTime(DateTime dateTime)
{
#ifdef _RASP
    char timeConfig[MAX_BUFFER_SIZE / 2] = "sudo date -s ";
    strcat(timeConfig, dateTime.data);
    pthread_mutex_lock(&mutex);

    errno = 0;
    system(timeConfig);
    if (errno != 0)
    {
        PERROR("Error to the return value of system function\n");
    }

    pthread_mutex_unlock(&mutex);
    TRACE("timeConfig : %s\n", timeConfig);
#endif
    TRACE("\033[33m Configuration de la date et de l'heure effectuée \033[0m \n");
}
#endif

#ifndef _WRAP_SETDATI
extern void ClockMaker_setDefaultDateTime(void)
{
#ifdef _RASP
    pthread_mutex_lock(&mutex);
    errno = 0;
    system("sudo date -s '2021-01-01 00:00:00'");
    if (errno != 0)
    {
        PERROR("Error to the return value of system function\n");
    }
    pthread_mutex_unlock(&mutex);
#endif

    TRACE("\033[33m Configuration de la date et de l'heure par défaut effectuée : 2021-01-01 00:00:00 \033[0m \n");
}
#endif

#ifndef _WRAP_SETDATI
extern DateTime ClockMaker_getCurrentDateTime(void)
{
    int8_t check;
    time_t timestamp = time(NULL);

    if (timestamp == -1)
    {
        PERROR("Error to the return value of time()");
    }

    pthread_mutex_lock(&mutex);

    errno = 0;
    struct tm *now = localtime(&timestamp);
    if (errno != 0)
    {
        PERROR("Error to the return value of localtime");
    }

    check = sprintf(myCurrentDateTime.data, "%4d-%02d-%02d %02d:%02d:%02d",
                    now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
                    now->tm_hour, now->tm_min, now->tm_sec);

    if (check < 0)
    {
        PERROR("Error to the return value of sprintf");
    }

    pthread_mutex_unlock(&mutex);
    return myCurrentDateTime;
}
#endif
