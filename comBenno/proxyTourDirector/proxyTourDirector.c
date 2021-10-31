/**
 * @file proxyTourDirector.c
 * @author Léo Chauvin
 * @brief proxyTourDirector allows to receive the initial tour data from tourDirector and to send back the current tour data.
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

#include "proxyTourDirector.h"
#include "./../postmanBenno/postmanBenno.h"
#include "./../../utils.h"


extern void ProxyTourDirector_setCurrentBinData(BinData binData)
{
    char completedBinData[MAX_BUFFER_SIZE] = "STEP:";
    const char * binDataString = json_object_to_json_string(binData);
    strncat(completedBinData, binDataString, MAX_BUFFER_SIZE);
    PostmanBenno_sendMsg(completedBinData);
}

extern void ProxyTourDirector_setCurrentTourData(TourData tourData)
{
    char completedTourData[MAX_BUFFER_SIZE] = "TOUR:";
    const char * tourDataString = json_object_to_json_string(tourData);
    strncat(completedTourData, tourDataString, MAX_BUFFER_SIZE);
    PostmanBenno_sendMsg(completedTourData);
}
