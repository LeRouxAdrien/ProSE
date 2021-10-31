/**
 * @file liftingButton.h
 * @author Adrien LE ROUX
 * @brief 
 * @version 0.1
 * @date 2021-06-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef LIFTING_BUTTON_H
#define LIFTING_BUTTON_H
/**
 * @brief initialize the port for the lifting Button.
 * 
 */
extern void LiftingButton_init ();

/**
 * @brief free the port used for the lifting button.
 * 
 */
extern void LiftingButton_free ();

/**
 * @brief return the push button state 
 * 
 */
extern int LiftingButton_getStateReadButton ();

/**
 * @brief start the read polling process of the lifting push button
 * 
 */
extern void LiftingButton_startPolling ();

/**
 * @brief stop the read polling process of the lifting push button
 * 
 */
extern void LiftingButton_stopPolling ();

#endif