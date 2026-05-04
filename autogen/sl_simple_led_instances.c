/***************************************************************************//**
 * @file
 * @brief LED Driver Instances
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "sl_simple_led.h"
#include "sl_gpio.h"

#include "sl_simple_led_blue_config.h"
#include "sl_simple_led_green_config.h"
#include "sl_simple_led_red_config.h"

sl_simple_led_context_t simple_blue_context = {
  .port = SL_SIMPLE_LED_BLUE_PORT,
  .pin = SL_SIMPLE_LED_BLUE_PIN,
  .polarity = SL_SIMPLE_LED_BLUE_POLARITY,
};

const sl_led_t sl_led_blue = {
  .context = &simple_blue_context,
  .init = sl_simple_led_init,
  .turn_on = sl_simple_led_turn_on,
  .turn_off = sl_simple_led_turn_off,
  .toggle = sl_simple_led_toggle,
  .get_state = sl_simple_led_get_state,
};
sl_simple_led_context_t simple_green_context = {
  .port = SL_SIMPLE_LED_GREEN_PORT,
  .pin = SL_SIMPLE_LED_GREEN_PIN,
  .polarity = SL_SIMPLE_LED_GREEN_POLARITY,
};

const sl_led_t sl_led_green = {
  .context = &simple_green_context,
  .init = sl_simple_led_init,
  .turn_on = sl_simple_led_turn_on,
  .turn_off = sl_simple_led_turn_off,
  .toggle = sl_simple_led_toggle,
  .get_state = sl_simple_led_get_state,
};
sl_simple_led_context_t simple_red_context = {
  .port = SL_SIMPLE_LED_RED_PORT,
  .pin = SL_SIMPLE_LED_RED_PIN,
  .polarity = SL_SIMPLE_LED_RED_POLARITY,
};

const sl_led_t sl_led_red = {
  .context = &simple_red_context,
  .init = sl_simple_led_init,
  .turn_on = sl_simple_led_turn_on,
  .turn_off = sl_simple_led_turn_off,
  .toggle = sl_simple_led_toggle,
  .get_state = sl_simple_led_get_state,
};

const sl_led_t *sl_simple_led_array[] = {
  &sl_led_blue,
  &sl_led_green,
  &sl_led_red
};

void sl_simple_led_init_instances(void)
{
  sl_led_init(&sl_led_blue);
  sl_led_init(&sl_led_green);
  sl_led_init(&sl_led_red);
}
