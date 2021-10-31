/**
 * @file ProxyTourDirector.h
 * @author Léo Chauvin
 * @brief the header file of ProxyTourDirector.c
 * @version 2.0
 * @date 2021-04-27
 * @copyright © 2021, Turfly company.
 * 
 */

#ifndef PROXY_TOUR_DIRECTOR_H
#define PROXY_TOUR_DIRECTOR_H
#include <stdio.h>
#include <stdint.h>
#include "./../../common.h"

/**
 * @brief Send formatted data to postmanBenno with the binData received
 * 
 * @param binData The binData received from tourManager
 */
extern void ProxyTourDirector_setCurrentBinData(BinData binData);

/**
 * @brief Send formatted data to postmanBenno with the tourData received
 * 
 * @param tourData The tourData received from tourManager
 */
extern void ProxyTourDirector_setCurrentTourData(TourData tourData);

#endif /* PROXY_TOUR_DIRECTOR_H */