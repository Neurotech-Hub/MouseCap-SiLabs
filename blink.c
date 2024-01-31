#include "sl_simple_led.h"
#include "sl_simple_led_instances.h"
#include "sl_sleeptimer.h"
#include "config.h"
/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

sl_sleeptimer_timer_handle_t timer;
bool toggle_timeout = false;
bool should_blink = false; // Global flag to control blinking

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

static void
on_timeout (sl_sleeptimer_timer_handle_t *handle, void *data);

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize blink example.
 ******************************************************************************/
void
blink_init (void)
{
  // Initialize related components here but do not start the timer yet.
}

/***************************************************************************//**
 * Function to start blinking.
 ******************************************************************************/
void
start_blinking (void)
{
  sl_status_t status;
  bool isRunning = false;

  should_blink = true;
  // Check if the timer is running
  status = sl_sleeptimer_is_timer_running (&timer, &isRunning);

  if (status == SL_STATUS_OK && !isRunning)
    {
      // Timer is not running, so start it
      sl_sleeptimer_start_periodic_timer_ms (
          &timer,
          TOOGLE_DELAY_MS,
          on_timeout, NULL, 0,
          SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
    }
}

/***************************************************************************//**
 * Function to stop blinking.
 ******************************************************************************/
void
stop_blinking (void)
{
  should_blink = false;
  // Stop the timer
  sl_sleeptimer_stop_timer (&timer);
}

/***************************************************************************//**
 * Blink ticking function.
 ******************************************************************************/
void
blink_process_action (void)
{
  if (toggle_timeout && should_blink)
    {
      sl_led_toggle (LED_INSTANCE);
      toggle_timeout = false;
    }
}

/***************************************************************************//**
 * Sleeptimer timeout callback.
 ******************************************************************************/
static void
on_timeout (sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void) &handle;
  (void) &data;
  toggle_timeout = true;
}
