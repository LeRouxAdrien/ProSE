/**
 * @file forcingButton.h
 * @author Adrien LE ROUX
 * @brief 
 * @version 0.1
 * @date 2021-06-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef FORCING_BUTTON_H
#define FORCING_BUTTON_H
/**
 * @brief initialize the port for the forcing Button.
 * 
 */
extern void ForcingButton_init ();

/**
 * @brief free the port used for the forcing button.
 * 
 */
extern void ForcingButton_free ();

/**
 * @brief return the push button state 
 * 
 */
extern int ForcingButton_getStateReadButton ();

/**
 * @brief start the read polling process of the forcing push button
 * 
 */
extern void ForcingButton_startPolling ();

/**
 * @brief stop the read polling process of the forcing push button
 * 
 */
extern void ForcingButton_stopPolling ();

#endif