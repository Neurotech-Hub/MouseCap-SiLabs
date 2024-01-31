/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "app_log.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "sl_simple_led_instances.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "blink.h"
#include "config.h"
#include <stdio.h>

#define COMMAND_STR_MAX_SIZE 20 // should match nodeTx characteristic size

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static uint8_t activateOnDisconnect = 0;
static int amplitude = 0;
static int frequency = 0;
static int pulseWidth = 0;

void
handleNodeRxChange (uint8_t *data, size_t len);
void
compileCommandString (char *commandStr);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void
app_init (void)
{
  blink_init ();
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void
app_process_action (void)
{
  blink_process_action ();
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void
sl_bt_on_event (sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header))
    {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set (&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data (
          advertising_set_handle, sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing (advertising_set_handle, 160, // min. adv. interval (milliseconds * 1.6)
                                        160, // max. adv. interval (milliseconds * 1.6)
                                        0,   // adv. duration
                                        0);  // max. num. adv. events
      app_assert_status(sc);

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start (advertising_set_handle,
                                          sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

      // -------------------------------
      // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log_info("Connection opened.\n");

      activateOnDisconnect = 0; // reset
      stop_blinking ();
      sl_led_turn_off (LED_INSTANCE); // known state
      break;

      // -------------------------------
      // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log_info("Connection closed.\n");

      sl_led_turn_off (LED_INSTANCE); // known state
      if (activateOnDisconnect)
        {
          start_blinking ();
        }

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data (
          advertising_set_handle, sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start (advertising_set_handle,
                                          sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

      // -------------------------------
      // This event indicates that the value of an attribute in the local GATT
      // database was changed by a remote GATT client.
    case sl_bt_evt_gatt_server_attribute_value_id:
      // The value of the gattdb_led_control characteristic was changed.

      // Check if the attribute written is gattdb_node_rx
      if (evt->data.evt_gatt_server_attribute_value.attribute == gattdb_node_rx)
        {
          // Read the updated value
          uint8_t buffer[20]; // Adjust the size as needed
          size_t len = 0;
          sc = sl_bt_gatt_server_read_attribute_value (
          gattdb_node_rx,
                                                       0, sizeof(buffer), &len,
                                                       buffer);

          if (sc == SL_STATUS_OK)
            {
              // Trigger your function here and pass the buffer and len as parameters
              handleNodeRxChange (buffer, len);

              // get command string and set nodeTxx
              char commandStr[COMMAND_STR_MAX_SIZE];
              compileCommandString (commandStr);
              // Write attribute in the local GATT database.
              sc = sl_bt_gatt_server_write_attribute_value (
                  gattdb_node_tx, 0, sizeof(commandStr), (uint8_t*) commandStr);
            }
          else
            {
              // Handle error
            }
        }
      break;

      // -------------------------------
      // This event occurs when the remote device enabled or disabled the
      // notification.
    case sl_bt_evt_gatt_server_characteristic_status_id:
      break;

      ///////////////////////////////////////////////////////////////////////////
      // Add additional event handlers here as your application requires!      //
      ///////////////////////////////////////////////////////////////////////////

      // -------------------------------
      // Default event handler.
    default:
      break;
    }
}

void
compileCommandString (char *commandStr)
{
  if (commandStr != NULL)
    {
      // Initialize the entire buffer with null characters
      memset (commandStr, 0, COMMAND_STR_MAX_SIZE);
      // Format the string with the global variable values
      sprintf(commandStr, "_A%u,F%u,P%u,G%u", amplitude, frequency, pulseWidth, activateOnDisconnect);
    }
}

void
handleNodeRxChange (uint8_t *data, size_t len)
{
  if (len == 0 || data[0] != '_')
    {
      return; // No valid data or does not start with '_'
    }

  // Convert data to null-terminated string
  char str[len + 1]; // +1 for null terminator
  memset (str, 0, len + 1); // Initialize buffer to zero
  memcpy (str, data, len);
  str[len] = '\0';

  // Tokenize the string
  char *token = strtok (str + 1, ","); // Start from str + 1 to skip '_'
  while (token != NULL)
    {
      char settingType = token[0];
      int value = atoi (token + 1); // Convert string to integer

      switch (settingType)
        {
        case 'A':
          amplitude = value;
          break;
        case 'F':
          frequency = value;
          break;
        case 'P':
          pulseWidth = value;
          break;
        case 'L':
          if (value)
            {
              sl_led_toggle (LED_INSTANCE);
            }
          break;
        case 'G':
          activateOnDisconnect = value;
          break;
        default:
          // Handle unknown setting
          break;
        }

      token = strtok (NULL, ","); // Get next token
    }
}

