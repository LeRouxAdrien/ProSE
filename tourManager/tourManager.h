/**
 * @file tourManager.h
 *
 * @author Damien Frissant
 * 
 * @brief the header file of tourManager.c
 * 
 * @version 2.0
 * 
 * @date 2021-04-27
 * 
 * @copyright © 2021, Turfly company.
 * 
 */

#ifndef TOUR_MANAGER_H
#define TOUR_MANAGER_H
#include <stdio.h>
#include <stdint.h>
#include "./../scales/scales.h"
#include "./../uiBenno/speaker/speaker.h"
#include "./../common.h"
#include "./../binScanner/scanner/scanner.h"
#include "./../clockMaker/clockMaker.h"


/**
 * @brief Start the routine of the thread to run the state machine
 * @see *run()
 * 
 */
extern void TourManager_start(void);

/**
 * @brief Kill the active object
 * 
 */
extern void TourManager_stop(void);

/**
 * @brief Send a text message on the message queue when a TourData is set
 * 
 * @param tourData is a JSON file. 
 */
extern void TourManager_setInitialTourData(TourData myCurrentTourData);

/**
 * @brief Send a text message on the message queue when the idBin is set
 * 
 * @param idBin is a uint32_t, after translated into hexadecimal, this variable will last 8 characteres. This is the id of a bin
 */
extern void TourManager_setIdBinToTourManager(IdBin myCurrentIdBin);

/**
 * @brief Send a text message on the message queue when the weight is set
 * 
 * @param weight of a bin. This is an integer between 0 to 500 kg
 */
extern void TourManager_setWeight(Weight myCurrentWeight);

/**
 * @brief Send a text message on the message queue when TourData is acknowledge 
 * 
 */
extern void TourManager_ackTourData(void);

/**
 * @brief Send a text message on the message queue when the empty time is done
 * 
 */
extern void TourManager_timeOutEmptyTime(void);

/**
 * @brief Request to stop BinScanner, call TourManager_stop() after that
 * 
 */
extern void TourManager_requestToStopTools(void);

/**
 * @brief Request to stop BinScanner, call TourManager_stop() after that
 * 
 */
extern void TourManager_requestToAskWeight(void);

/**
 * @brief Destroys the message queue
 * 
 */
extern void TourManager_free(void);

/**
 * @brief Initializes the message queue
 * 
 */
extern void TourManager_new(void);

/**
 * @brief Send a text message on the message queue the signal to stop the current tour
 * 
 */
extern void TourManager_signalEndTour(void);

/**
 * @brief Send a text message on the message queue to begin the rest
 * 
 */
extern void TourManager_startRest(void);

/**
 * @brief Send a text message on the message queue to end the rest
 * 
 */
extern void TourManager_stopRest(void);

/**
 * @brief Start Speaker, ClockMaker and TourManager
 * 
 */
extern void TourManager_startBenno(void);


#endif /* TOUR_MANAGER_H */