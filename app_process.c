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
#include "sl_hal_wdog.h"
// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------
#define MAX_TX_FAILURES     (10U)
#define TICKS_5S  160000
#define TICKS_10S 320000
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
extern EmberEventControl *TimeoutStatus_control;
/// report timing period
uint16_t sensor_report_period_ms =  (1 * MILLISECOND_TICKS_PER_SECOND);
/// TX options set up for the network
EmberMessageOptions tx_options = EMBER_OPTIONS_ACK_REQUESTED | EMBER_OPTIONS_SECURITY_ENABLED;

extern EmberKeyData connect_network_key;
extern uint8_t transport_key[16];

//NETWORK_SCAN
network_scan_t network_scan_result[12];
uint8_t network_count = 0;

extern send_queue_t radioQueue[MAX_QUEUE_PACKETS];

extern uint16_t SL_SENSOR_SINK_PAN_ID_RANDOM;

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------
/// Destination of the currently processed sink node
bool register_control;
uint32_t press_start_time = 0;
bool button_is_pressed = false;
uint32_t timer_counter = 200;

bool ok_to_blink = true;
bool initialized = false;
bool joining = false;

extern bool sensor_joining;
extern uint32_t join_timeout;

bool CHOQUE = false;

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

void app_init(){
  emberAfAllocateEvent(&radio_control, &radio_handler);
  emberAfAllocateEvent(&CheckState_control, &CheckState_handler);
  emberAfAllocateEvent(&report_control, &report_handler);
  emberAfAllocateEvent(&Init_control, &Init_handler);
  connect_init();

  emberEventControlSetDelayMS(*Init_control, 100);
}

void Init_handler(){
  emberAfPluginPollEnableShortPolling(true);

  memory_read(MODULE_MODE_MEMORY_KEY, &application.Module_mode);
  memory_read(STATUSOP_MEMORY_KEY, &application.Status_Operation);
  memory_read(SECURITY_KEY_MEMORY_KEY, &connect_network_key.contents);
  memory_read(PAN_ID_MEMORY_KEY, &SL_SENSOR_SINK_PAN_ID_RANDOM);

  application.device_info.device_type = 0;
  application.device_info.device_type_identified = false;

  emberEventControlSetDelayMS(*CheckState_control,2000);

  app_wdog_init();

  initialized = true;

  emberEventControlSetInactive(*Init_control);
}

void sl_button_on_change(const sl_button_t *handle)
{

    if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
       if(&sl_button_btn0 == handle){
           //BOTAO PRESSIONADO
           press_start_time = sl_sleeptimer_get_tick_count();
           button_is_pressed = true;
       }
    }

    if(sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED){
        //BOTAO SOLTO
        uint32_t current_time = sl_sleeptimer_get_tick_count();
        button_is_pressed = false;

        uint32_t press_elapsed_time = current_time - press_start_time;

        if(((press_elapsed_time) > TICKS_5S) && ((press_elapsed_time) < TICKS_10S)){
            //RESET NETWORK
            emberResetNetworkState();
            reset_parameters();

        }else if((press_elapsed_time) > TICKS_10S){
            //CHANGE MODE
            if(application.Module_mode == ALARM_MODE){
                application.Module_mode = STANDALONE_RECEPTOR;
                memory_write(MODULE_MODE_MEMORY_KEY, &application.Module_mode, sizeof(application.Module_mode));
                led_blink(VERDE, 5, FAST_SPEED_BLINK);
            }else if(application.Module_mode == STANDALONE_RECEPTOR){
                application.Module_mode = ALARM_MODE;
                memory_write(MODULE_MODE_MEMORY_KEY, &application.Module_mode, sizeof(application.Module_mode));
                led_blink(VERDE, 5, FAST_SPEED_BLINK);
            }
            emberResetNetworkState();
            reset_parameters();

        }else if((press_elapsed_time) <= TICKS_5S){
            //ACTION
            if(application.Status_Operation == WAIT_REGISTRATION){
                if(application.Module_mode == ALARM_MODE){
                    startActiveScanCommand();
                    ok_to_blink = false;
                    sl_led_turn_on(&sl_led_blue);
                }else if(application.Module_mode == STANDALONE_RECEPTOR){
                    if(!joining){
                        joining = true;
                        form_network();
                    }else{
                        Network_join_timeout();
                    }
                }
            }else{
                if(!CHOQUE){
                    CHOQUE = true;
                    application.radio.Packet.cmd = LRCMD_WRITE_CONTROL_CERCA;
                    application.radio.Packet.data[0] = LIGA_PGM;
                    emberEventControlSetDelayMS(*radio_control, 100);
                    emberEventControlSetDelayMS(*TimeoutStatus_control,2000);
                }else{
                    CHOQUE = false;
                    application.radio.Packet.cmd = LRCMD_WRITE_CONTROL_CERCA;
                    application.radio.Packet.data[0] = DESLIGA_PGM;
                    emberEventControlSetDelayMS(*radio_control, 100);
                    emberEventControlSetDelayMS(*TimeoutStatus_control,2000);
                }
            }
        }
    }
}

void reset_parameters(){
  memory_read(MODULE_MODE_MEMORY_KEY, &application.Module_mode);
  memory_erase(STATUSOP_MEMORY_KEY);
  memory_erase(SECURITY_KEY_MEMORY_KEY);
  memory_erase(DEVICE_TYPE_MEMORY_KEY);
  memory_erase(PAN_ID_MEMORY_KEY);

  psa_status_t psa_status;
  sl_se_command_context_t cmd_ctx;
  sl_status_t status;

  psa_status = psa_generate_random(connect_network_key.contents, EMBER_ENCRYPTION_KEY_SIZE);

  if (psa_status == PSA_SUCCESS) {
      if (set_security_key(connect_network_key.contents, (size_t)EMBER_ENCRYPTION_KEY_SIZE) == true) {
          memory_write(SECURITY_KEY_MEMORY_KEY, connect_network_key.contents, sizeof(connect_network_key.contents));
      } else {
  //      app_log_error("Connect: set the random key as network key failed\n");
      }
  } else {
  //      app_log_error("PSA: generate random network key failed (status: %ld)\n", psa_status);
  }

  status = sl_se_get_random(&cmd_ctx, &SL_SENSOR_SINK_PAN_ID_RANDOM, sizeof(SL_SENSOR_SINK_PAN_ID_RANDOM));

  if(status == SL_STATUS_OK){
      memory_write(PAN_ID_MEMORY_KEY, (uint8_t *)&SL_SENSOR_SINK_PAN_ID_RANDOM, sizeof(SL_SENSOR_SINK_PAN_ID_RANDOM));
  }

  application.Status_Operation = WAIT_REGISTRATION;

  application.device_info.device_type_identified = false;

  memory_write(STATUSOP_MEMORY_KEY, &application.Status_Operation, sizeof(application.Status_Operation));
  memory_write(DEVICE_TYPE_MEMORY_KEY, (uint8_t *)&application.device_info, sizeof(application.device_info));
}

/**************************************************************************//**
 * Here we print out the first two bytes reported by the sinks as a little
 * endian 16-bits decimal.
 *****************************************************************************/
void report_handler(void)
{
  packet_void_t sendRadio;
  volatile Register_Sensor_t Register_Sensor;

  switch (application.Status_Operation){
    case WAIT_REGISTRATION:

      Register_Sensor.Status.Type = application.device_info.device_type;
      Register_Sensor.Status.range = LONG_RANGE;

      if(Register_Sensor.Status.Type == CERCA){
          sendRadio.cmd = LRCMD_JOINED_NETWORK_CERCA;
      }else if(Register_Sensor.Status.Type == GATE){
          sendRadio.cmd = LRCMD_JOINED_NETWORK_GATE;
      }

      sendRadio.len = 3;
      sendRadio.data[0] = Register_Sensor.Registerbyte;
      sendRadio.data[1] = application.Connect_ID;
      application.radio.LastCMD = sendRadio.cmd;
      radio_send_packet(&sendRadio, 0, false);
      break;

    case OPERATION_MODE:
      if(application.radio.LastCMD == LRCMD_CHANGE_STATUS_GATE){
          get_state_gate(&application.state_gate);

          sendRadio.cmd = LRCMD_CHANGE_STATUS_GATE;
          sendRadio.len = 6;
          sendRadio.data[0] = application.state_gate;         //Status portao
          sendRadio.data[1] = 0xFF;                              //NULL
          sendRadio.data[2] = 0xFF;                              //NULL
          sendRadio.data[3] = 0xFF;                              //NULL
          sendRadio.data[4] = application.radio.RSSI;
          radio_send_packet(&sendRadio, 0, false);
      }

      if(application.radio.LastCMD == LRCMD_STATUS_CENTRAL){
          get_state_gate(&application.state_gate);

          sendRadio.cmd = LRCMD_STATUS_CENTRAL;
          sendRadio.len = 6;
          sendRadio.data[0] = application.state_gate;         //Status portao
          sendRadio.data[1] = 0xFF;                              //NULL
          sendRadio.data[2] = 0xFF;                              //NULL
          sendRadio.data[3] = 0xFF;                              //NULL
          sendRadio.data[4] = application.radio.RSSI;
          radio_send_packet(&sendRadio, 0, false);
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
  if((message->payload[0] == LRCMD_JOINED_NETWORK_GATE || message->payload[0] == LRCMD_JOINED_NETWORK_CERCA) && application.Status_Operation == WAIT_REGISTRATION && status == EMBER_SUCCESS){
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
              if(application.Module_mode == ALARM_MODE){
                  if(radioQueue[i].attempts == MAX_QUEUE_PACKET_ATTEMPTS){
                      radioQueue[i].slot_used = false;
                      radioQueue[i].attempts = 0;
                  }else{
                      radioQueue[i].slot_used = true;
                      radioQueue[i].attempts++;
                      radio_send_packet(&radioQueue[i].packet, message->destination, true);
                  }
              }else if(application.Module_mode == STANDALONE_RECEPTOR){
                  radioQueue[i].slot_used = false;
                  radioQueue[i].attempts = 0;
              }
          }
      }
  }

  application.radio.RSSI = -(message->ackRssi);
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

void Network_join_timeout(){
  emberPermitJoining(0);
  joining = false;
  join_timeout = 0;

  emberSetUnencryptedPacketsAcceptance(false);
}

/**************************************************************************//**
 * This callback is called in each iteration of the main application loop and
 * can be used to perform periodic functions.
 *****************************************************************************/
void emberAfTickCallback(void)
{
  uint32_t current_time = sl_sleeptimer_get_tick_count();
  uint32_t press_elapsed_time = current_time - press_start_time;

  if(initialized){
      sl_hal_wdog_feed(WDOG0);

      if(((press_elapsed_time) > TICKS_5S) && button_is_pressed && ((press_elapsed_time) <= TICKS_10S)){
          //BOTAO PRESSIONADO ENTRE 5s e 10s
          sl_led_turn_off(&sl_led_red);
          sl_led_turn_off(&sl_led_green);
          sl_led_turn_off(&sl_led_blue);
      }else if(button_is_pressed && ((press_elapsed_time) > TICKS_10S)){
          //BOTAO PRESSIONADO POR MAIS DE 10s
          hGpio_ledTurnOn(&sl_led_green, false);
      }else{

          if(application.Module_mode == ALARM_MODE){
              //CONFIGURADO PARA FUNCIONAR NO ALARME
              if(emberStackIsUp()){
                  hGpio_ledTurnOn(&sl_led_green, false);
              }else {
                  if(ok_to_blink){
                      if((current_time - timer_counter) > 7800){
                          timer_counter = current_time;
                          hGpio_ledToggle(&sl_led_blue, false);
                          hGpio_ledTurnOff(&sl_led_red);
                          hGpio_ledTurnOff(&sl_led_green);
                      }
                  }
              }
          }else if(application.Module_mode == STANDALONE_RECEPTOR){
              //CONFIGURADO PARA FUNCIONAR COMO RECEPTOR AVULSO
              if (emberStackIsUp()) {
                  if(join_timeout > 1){
                      if((current_time - timer_counter) > 7800){
                          timer_counter = current_time;
                          hGpio_ledToggle(&sl_led_blue, false);
                          hGpio_ledTurnOff(&sl_led_red);
                          hGpio_ledTurnOff(&sl_led_green);
                      }
                      join_timeout--;
                  }else if(join_timeout == 1){
                      Network_join_timeout();
                  }else{
                      hGpio_ledTurnOn(&sl_led_green, false);
                  }
              }else {
                  hGpio_ledTurnOff(&sl_led_red);
                  hGpio_ledTurnOff(&sl_led_green);
                  hGpio_ledTurnOff(&sl_led_blue);
              }
          }
      }
  }
}

void emberAfChildJoinCallback(EmberNodeType nodeType,
                              EmberNodeId nodeId)
{
  app_log_info("Sensor joined with node ID 0x%04X, node type: 0x%02X\n", nodeId, nodeType);

  packet_void_t sendRadio;
  EmberKeyData encrypted_key;

  aes_ecb_encrypt_key(transport_key, connect_network_key.contents, encrypted_key.contents);

  sendRadio.cmd = LRCMD_SEND_KEY;
  sendRadio.len = 17;
  memcpy(sendRadio.data,encrypted_key.contents,EMBER_ENCRYPTION_KEY_SIZE);

  radio_send_packet(&sendRadio,nodeId,false);

  Network_join_timeout();
}

void startActiveScanCommand(void){
  EmberStatus status;
  uint8_t channelToScan = 0;
  status = emberStartActiveScan(channelToScan);

  if (status != EMBER_SUCCESS) {
      app_log_error("Start active scanning failed, status=0x%x", status);
  } else {
      app_log_error("Start active scanning: channel %d", channelToScan);
  }
}

void emberAfIncomingBeaconCallback(EmberPanId panId,
                                   EmberMacAddress *source,
                                   int8_t rssi,
                                   bool permitJoining,
                                   uint8_t beaconFieldsLength,
                                   uint8_t *beaconFields,
                                   uint8_t beaconPayloadLength,
                                   uint8_t *beaconPayload){
  (void) source;
  (void) beaconFields;
  (void) beaconFieldsLength;
  (void) beaconPayload;
  (void) beaconPayloadLength;

  network_scan_result[network_count].PanID = panId;
  network_scan_result[network_count].rssi = rssi;
  network_scan_result[network_count].permitJoining = permitJoining;
  network_count++;
}

void emberAfActiveScanCompleteCallback(){
  int8_t best_rssi = -128;  // Valor mínimo de RSSI
  uint8_t best_index = 0xFF;  // Índice inválido
  bool found_network = false;

  // Percorre todas as redes encontradas
  for(uint8_t i = 0; i < 12; i++){
      if(network_scan_result[i].permitJoining){
          // Se é a primeira rede válida ou tem RSSI melhor que a anterior
          if(!found_network || network_scan_result[i].rssi > best_rssi){
              best_rssi = network_scan_result[i].rssi;
              best_index = i;
              found_network = true;
          }
      }
  }

  // Se encontrou pelo menos uma rede com permit join, conecta na melhor
  if(found_network && best_index != 0xFF){
      join_sleepy(network_scan_result[best_index].PanID);
  }

  // Limpa o contador para o próximo scan
  network_count = 0;
}

void app_wdog_init(void)
{
  sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_WDOG0);

  sl_hal_wdog_init_t init = SL_HAL_WDOG_INIT_DEFAULT;

  // Para Sleepy End Device: WDOG congela durante sleep
  init.em2_run          = false;
  init.em3_run          = false;
  init.period_select    = SL_WDOG_PERIOD_513;         // ~2s com ULFRCO
  init.warning_time_select = SL_WDOG_WARNING_TIME75;  // Aviso em 75%

  sl_hal_wdog_init(WDOG0, &init);

  sl_hal_wdog_clear_interrupts(WDOG0, WDOG_IF_WARN);
  sl_hal_wdog_enable_interrupts(WDOG0, WDOG_IF_WARN);

  sl_interrupt_manager_clear_irq_pending(WDOG0_IRQn);
  sl_interrupt_manager_enable_irq(WDOG0_IRQn);

  sl_hal_wdog_enable(WDOG0);
}
