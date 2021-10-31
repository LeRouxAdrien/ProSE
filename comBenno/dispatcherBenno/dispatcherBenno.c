/**
 * @file dispatcherBenno.c
 * @author Léo Chauvin
 * @brief
 * @version 2.0
 * @date 2021-04-27
 * 
 * @copyright © 2021, Turfly company.
 * 
 */
#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <json-c/json.h>
#include <stddef.h>
#include <errno.h>

#include "dispatcherBenno.h"
#include "./../../tourManager/tourManager.h"
#include "./../../uiBenno/speaker/speaker.h"
#include "./../../clockMaker/clockMaker.h"
#include "./../../utils.h"

static char firstFourChar[5];
TourData tourData;
DateTime dateTime;

/**
 * @brief Create a json file from a buffer
 * 
 */
static void writeTourData(char *buffer);

extern void DispatcherBenno_setMsg(char *receiveBuffer)
{
    memcpy(firstFourChar, receiveBuffer, 4 * sizeof(char));

    if (strcmp(firstFourChar, "CSOK") == 0)
    {
        TRACE("\033[34mSignal CSOK reçu \033[0m \n");
        Speaker_requestConnectionSignalOk();
    }
    if (strcmp(firstFourChar, "STOP") == 0)
    {
        TRACE("\033[34mSignal STOP reçu \033[0m \n");
        TourManager_signalEndTour();
    }
    if (strcmp(firstFourChar, "ACKN") == 0)
    {
        TRACE("\033[34mSignal ACKN reçu \033[0m \n");
        TourManager_ackTourData();
    }
    else if (strcmp(firstFourChar, "DATI") == 0)
    {
        TRACE("\033[34mSignal DATI reçu \033[0m \n");
        strncpy(dateTime.data, receiveBuffer + 5, sizeof(dateTime.data));
        ClockMaker_setCurrentDateTime(dateTime);
    }
    else if (strcmp(firstFourChar, "TOUR") == 0)
    {
        TRACE("\033[34mSignal TOUR reçu \033[0m \n");
        memmove(receiveBuffer, receiveBuffer + 5, MAX_BUFFER_SIZE);
        writeTourData(receiveBuffer);
        TourManager_setInitialTourData(tourData);
    }
    else if (strcmp(firstFourChar, "REST") == 0)
    {
        TRACE("\033[34mSignal REST reçu \033[0m \n");
        TourManager_startRest();
    }
    else if (strcmp(firstFourChar, "WAKE") == 0)
    {
        TRACE("\033[34mSignal WAKE reçu \033[0m \n");
        TourManager_stopRest();
    }
    else if (strcmp(firstFourChar, "") == 0)
    {
        TRACE("\033[34mLe signal reçu est vide. \033[0m \n");
    }
    else
    {
        TRACE("\033[34mLe message reçu ne correspond à aucun type de données. \033[0m \n");
    }
}

static void writeTourData(char *receiveBuffer)
{
    const char *filename = "./myCurrentTourData.json";
    tourData = json_tokener_parse(receiveBuffer);
    json_object_to_file_ext(filename, tourData, JSON_C_TO_STRING_PRETTY);
}