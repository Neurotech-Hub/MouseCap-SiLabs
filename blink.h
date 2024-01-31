/***************************************************************************//**
 * @file
 * @brief Blink examples functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef BLINK_H
#define BLINK_H

#include "sl_sleeptimer.h"

/**
 * @brief Initialize blink functionality.
 *
 * This function initializes resources needed for the blink functionality
 * but does not start the blinking process.
 */
void blink_init(void);

/**
 * @brief Start the LED blinking process.
 *
 * This function enables the LED to start blinking at a predefined interval.
 */
void start_blinking(void);

/**
 * @brief Stop the LED blinking process.
 *
 * This function stops the LED from blinking and turns off the LED.
 */
void stop_blinking(void);

/**
 * @brief Process blink action.
 *
 * This function checks if it's time to toggle the LED state and performs
 * the toggle if necessary. It should be called periodically to ensure
 * the blinking functionality works.
 */
void blink_process_action(void);

#endif // BLINK_H
