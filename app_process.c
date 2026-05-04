/***************************************************************************//**
 * @file
 * @brief app_process.c
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
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

// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include PLATFORM_HEADER
#include "stack/include/ember.h"
#include "em_chip.h"
#include "app_log.h"
#include "poll.h"
#include "sl_app_common.h"
#include "app_process.h"
#include "sl_sleeptimer.h"
#include "app_framework_common.h"
#if defined(SL_CATALOG_LED0_PRESENT)
#include "sl_simple_led_instances.h"
#endif
#include "sl_simple_button_instances.h"
#if defined(SL_CATALOG_KERNEL_PRESENT)
#include "sl_component_catalog.h"
#include "sl_power_manager.h"
#endif
#include "API/hNetwork.h"
#include "Application/application.h"
#include "API/packet/pckDataStructure.h"
// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------
#define MAX_TX_FAILURES     (10U)
// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------
/// Global flag set by a button push to allow or disallow entering to sleep
bool enable_sleep = false;
/// report timing event control
EmberEventControl *report_control;
EmberEventControl *radio_control;
EmberEventControl *CheckState_control;
EmberEventControl *Init_control;
/// report timing period
uint16_t sensor_report_period_ms =  (1 * MILLISECOND_TICKS_PER_SECOND);
/// TX options set up for the network
EmberMessageOptions tx_options = EMBER_OPTIONS_ACK_REQUESTED | EMBER_OPTIONS_SECURITY_ENABLED;

extern EmberKeyData connect_network_key;

extern packet_void_t sendRadio;
extern send_queue_t radioQueue[MAX_QUEUE_PACKETS];

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------
/// Destination of the currently processed sink node
static EmberNodeId sink_node_id = EMBER_COORDINATOR_ADDRESS;
bool register_control;
uint32_t press_start_time = 0;
bool button_is_pressed = false;
uint32_t timer_counter = 200;

bool ok_to_blink = true;
bool initialized = false;
bool joining = false;
// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

void app_init(){
  emberAfAllocateEvent(&radio_control, &radio_handler);
  emberAfAllocateEvent(&CheckState_control, &CheckState_handler);
  emberAfAllocateEvent(&report_control, &report_handler);
  emberAfAllocateEvent(&Init_control, &Init_handler);
  connect_init();

  emberEventControlSetDelayMS(*Init_control, 1000);
}

void Init_handler(){
  emberAfPluginPollEnableShortPolling(true);

  memory_read(MODULE_MODE_MEMORY_KEY, &application.Module_mode);
  memory_read(STATUSOP_MEMORY_KEY, &application.Status_Operation);
  memory_read(SECURITY_KEY_MEMORY_KEY, &connect_network_key.contents);

  emberEventControlSetDelayMS(*CheckState_control,2000);

  initialized = true;

  emberEventControlSetInactive(*Init_control);
}

void sl_button_on_change(const sl_button_t *handle)
{

    if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
       if(&sl_button_btn0 == handle){
           press_start_time = sl_sleeptimer_get_tick_count();
           button_is_pressed = true;
       }
    }

    if(sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED){
        uint32_t current_time = sl_sleeptimer_get_tick_count();
        button_is_pressed = false;

        uint32_t elapsed_time = current_time - press_start_time;

        if(((elapsed_time) > 100000) && ((elapsed_time) < 200000)){
            //RESET NETWORK
            emberResetNetworkState();
            reset_parameters();

        }else if((elapsed_time) > 200000){
            //CHANGE MODE
            if(application.Module_mode == ALARM){
                application.Module_mode = RECEPTOR;
                memory_write(MODULE_MODE_MEMORY_KEY, &application.Module_mode, sizeof(application.Module_mode));
                led_blink(VERMELHO, 5, FAST_SPEED_BLINK);

            }else if(application.Module_mode == RECEPTOR){
                application.Module_mode = ALARM;
                memory_write(MODULE_MODE_MEMORY_KEY, &application.Module_mode, sizeof(application.Module_mode));
                led_blink(VERMELHO, 5, FAST_SPEED_BLINK);
            }

        }else if((elapsed_time) <= 100000){
            //ACTION
            if(application.Status_Operation == WAIT_REGISTRATION){
                if(application.Module_mode == ALARM){
                    join_sleepy(0);
                    ok_to_blink = false;
                    sl_led_turn_on(&sl_led_blue);
                }else if(application.Module_mode == RECEPTOR){
                    joining = true;
                    form_network();
                    led_blink(VERMELHO, 5, SLOW_SPEED_BLINK);
                }

            }
        }
    }
}

void reset_parameters(){
  memory_erase(STATUSOP_MEMORY_KEY);
  memory_erase(SECURITY_KEY_MEMORY_KEY);

  application.Status_Operation = WAIT_REGISTRATION;

  memory_write(STATUSOP_MEMORY_KEY, &application.Status_Operation, sizeof(application.Status_Operation));
}

/**************************************************************************//**
 * Here we print out the first two bytes reported by the sinks as a little
 * endian 16-bits decimal.
 *****************************************************************************/
void report_handler(void)
{
  packet_void_t sendRadio;
  volatile Register_Sensor_t Register_Sensor;

  switch (application.Status_Operation) {
    case WAIT_REGISTRATION:

      Register_Sensor.Status.Type = GATE;
      Register_Sensor.Status.range = LONG_RANGE;

      sendRadio.cmd = LRCMD_JOINED_NETWORK_GATE;
      sendRadio.len = 2;
      sendRadio.data[0] = Register_Sensor.Registerbyte;

      application.radio.LastCMD = sendRadio.cmd;
      radio_send_packet(&sendRadio, false);
      break;

    case OPERATION_MODE:
      if(application.radio.LastCMD == LRCMD_GATE_CHANGE_STATUS){
          get_state(&application.state_real);

          sendRadio.cmd = LRCMD_GATE_CHANGE_STATUS;
          sendRadio.len = 6;
          sendRadio.data[0] = application.state_real;         //Status portao
          sendRadio.data[1] = 0xFF;                              //NULL
          sendRadio.data[2] = 0xFF;                              //NULL
          sendRadio.data[3] = 0xFF;                              //NULL
          sendRadio.data[4] = application.radio.RSSI;
          radio_send_packet(&sendRadio, false);
      }

      if(application.radio.LastCMD == LRCMD_STATUS_CENTRAL){
          get_state(&application.state_real);

          sendRadio.cmd = LRCMD_STATUS_CENTRAL;
          sendRadio.len = 6;
          sendRadio.data[0] = application.state_real;         //Status portao
          sendRadio.data[1] = 0xFF;                              //NULL
          sendRadio.data[2] = 0xFF;                              //NULL
          sendRadio.data[3] = 0xFF;                              //NULL
          sendRadio.data[4] = application.radio.RSSI;
          radio_send_packet(&sendRadio, false);
      }
      break;
  }

  emberEventControlSetInactive(*report_control);
}

/**************************************************************************//**
 * This function is called to indicate whether an outgoing message was
 * successfully transmitted or to indicate the reason of failure.
 *****************************************************************************/
void emberAfMessageSentCallback(EmberStatus status,
                                EmberOutgoingMessage *message)
{
  if(message->payload[0] == LRCMD_JOINED_NETWORK_GATE && application.Status_Operation == WAIT_REGISTRATION && status == EMBER_SUCCESS){
      //Estado inicial do sensor apos cadastro
      application.Status_Operation = OPERATION_MODE;

      memory_write(STATUSOP_MEMORY_KEY, &application.Status_Operation, sizeof(application.Status_Operation));
  }

  for(int i = 0; i < MAX_QUEUE_PACKETS; i++){
      if(message->payload[0] == radioQueue[i].packet.cmd){
          if(status == EMBER_SUCCESS){
              radioQueue[i].slot_used = false;
              radioQueue[i].attempts = 0;
          }else{
              if(radioQueue[i].attempts == MAX_QUEUE_PACKET_ATTEMPTS){
                  radioQueue[i].slot_used = false;
                  radioQueue[i].attempts = 0;
              }else{
                  radioQueue[i].slot_used = true;
                  radioQueue[i].attempts++;
                  radio_send_packet(&radioQueue[i].packet, true);
              }
          }
      }
  }

  application.radio.RSSI = -(message->ackRssi);
}
void emberAfChildJoinCallback(EmberNodeType nodeType,
                              EmberNodeId nodeId){

}

/**************************************************************************//**
 * This function is called when the stack status changes.
 *****************************************************************************/
void emberAfStackStatusCallback(EmberStatus status)
{
  hGpio_ledTurnOff(&sl_led_blue);
  switch (status) {
    case EMBER_NETWORK_UP:
      // Schedule start of periodic sensor reporting to the Sink
//      if(application.Status_Operation == WAIT_REGISTRATION && initialized){
//          emberEventControlSetDelayMS(*report_control, sensor_report_period_ms);
//      }
      led_blink(VERDE,2,MED_SPEED_BLINK);
      break;
    case EMBER_NETWORK_DOWN:
      app_log_info("Network down\n");
      led_blink(VERMELHO,5,FAST_SPEED_BLINK);
      break;
    case EMBER_JOIN_SCAN_FAILED:
      app_log_error("Scanning during join failed\n");
      led_blink(VERMELHO,2,SLOW_SPEED_BLINK);
      break;
    case EMBER_JOIN_DENIED:
      app_log_error("Joining to the network rejected!\n");
      led_blink(VERMELHO,2,SLOW_SPEED_BLINK);
      break;
    case EMBER_JOIN_TIMEOUT:
      app_log_info("Join process timed out!\n");
      led_blink(VERMELHO,2,SLOW_SPEED_BLINK);
      break;
    default:
      app_log_info("Stack status: 0x%02X\n", status);
      led_blink(VERMELHO,2,SLOW_SPEED_BLINK);
      break;
  }
}

/**************************************************************************//**
 * This callback is called in each iteration of the main application loop and
 * can be used to perform periodic functions.
 *****************************************************************************/
void emberAfTickCallback(void)
{

  uint32_t current_time = sl_sleeptimer_get_tick_count();
  uint32_t elapsed_time = current_time - press_start_time;
  if(((elapsed_time) > 100000) && button_is_pressed && ((elapsed_time) <= 200000)){
      sl_led_turn_off(&sl_led_red);
      sl_led_turn_off(&sl_led_green);
      sl_led_turn_off(&sl_led_blue);
  }else if(button_is_pressed && ((elapsed_time) > 200000)){
      hGpio_ledTurnOn(&sl_led_red, false);
  }else{
      if(initialized){
          if(application.Module_mode == ALARM){
              if(emberStackIsUp()){
                  hGpio_ledTurnOn(&sl_led_green, false);
              }else {
                  if(ok_to_blink){
                      if((current_time - timer_counter) > 7800){
                          timer_counter = current_time;
                          sl_led_toggle(&sl_led_blue);
                          sl_led_turn_off(&sl_led_red);
                          sl_led_turn_off(&sl_led_green);
                      }
                  }
              }
          }else if(application.Module_mode == RECEPTOR){
              if(emberStackIsUp()){
                  if(!joining){
                      sl_led_turn_on(&sl_led_red);
                  }else{

                  }
              }else{

              }
          }
      }
  }
}
