/**
 * @file scales.h
 * @author Léo Chauvin
 * @brief the header file of scales.c
 * @version 2.0
 * @date 2021-04-27
 * @copyright © 2021, Turfly company.
 * 
 */

#ifndef SCALES
#define SCALES

/**
 * @brief Redefining the int type
 * 
 */
typedef int Weight;

/**
 * @brief Initialises the pins to get the weight of the bins 
 * 
 */
extern void Scales_new(void);

/**
 * @brief Frees the pins used for weighing the bins
 * 
 */
extern void Scales_free(void);

/**
 * @brief Asks the weight of the bin
 * 
 */
extern void Scales_askWeight(void);


#endif /* SCALES */