/**
 * @file stopButton.h
 * @author Adrien LE ROUX
 * @brief 
 * @version 0.1
 * @date 2021-06-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef STOP_BUTTON_H
#define STOP_BUTTON_H
/**
 * @brief initialize the port for the stop Button.
 * 
 */
extern void StopButton_init ();

/**
 * @brief free the port used for the stop button.
 * 
 */
extern void StopButton_free ();

/**
 * @brief return the push button state 
 * 
 */
extern int StopButton_getStateReadButton ();

/**
 * @brief start the read polling process of the stop push button
 * 
 */
extern void StopButton_startPolling ();

/**
 * @brief stop the read polling process of the stop push button
 * 
 */
extern void StopButton_stopPolling ();

#endif