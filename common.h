/**
 * @file common.h
 *
 * @author Léo Chauvin
 * 
 * @brief the header file of tourManager.c
 * 
 * @version 0.1
 * 
 * @date 2021-04-27
 * 
 * @copyright © 2021, Turfly company.
 * 
 */

#ifndef COMMON_H
#define COMMON_H

#define MAX_BUFFER_SIZE 1000
#define MAX_DATI_SIZE 21

/**
 * @brief Redefining the json_object structure to create the TourData type
 * 
 */
typedef struct json_object *TourData;

/**
 * @brief Redefining the json_object structure to create the BinData type
 * 
 */
typedef struct json_object *BinData;

#endif /* COMMON_H */
