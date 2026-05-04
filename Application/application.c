/*
 * application.c
 *
 *  Created on: 12 de ago. de 2024
 *      Author: diego.marinho
 */

#include "application.h"

application_t application;

extern EmberEventControl *radio_control;
extern EmberEventControl *report_control;
extern EmberEventControl *CheckState_control;

extern EmberMessageOptions tx_options;

EmberKeyData connect_network_key;

extern bool joining;
extern bool ok_to_blink;

packet_void_t sendRadio;
send_queue_t radioQueue[MAX_QUEUE_PACKETS];

sl_sleeptimer_timer_handle_t periodic_timer;
uint8_t blink_count = 0;
uint8_t blink_target = 0;
uint8_t led_target;

void CheckState_handler(void){
  EmberStatus status;
  get_state(&application.state_real);
  if(application.state_real != application.state_before){
      application.state_before = application.state_real;
      if(application.Module_mode == ALARM){
          app_log_info("Change status");

          sendRadio.cmd = LRCMD_GATE_CHANGE_STATUS;
          sendRadio.len = 6;
          sendRadio.data[0] = application.state_real;         //Status portao
          sendRadio.data[1] = 0xFF;                              //NULL
          sendRadio.data[2] = 0xFF;                              //NULL
          sendRadio.data[3] = 0xFF;                              //NULL
          sendRadio.data[4] = application.radio.RSSI;

          status = radio_send_packet(&sendRadio, false);
      }else if(application.Module_mode == RECEPTOR){
          sendRadio.cmd = LRCMD_GATE_CHANGE_STATUS;
          sendRadio.data[0] = application.state_before;
          sendRadio.len = 2;
          status = radio_send_packet(&sendRadio, false);
      }
  }

  emberEventControlSetDelayMS(*CheckState_control,300);
}

void radio_handler(void){
  EmberStatus status;
  volatile packet_void_t *receive;

  receive = &application.radio.Packet;

  application.radio.LastCMD = receive->cmd;

  switch(receive->cmd){
    case LRCMD_WRITE_CONTROL_GATE:
      if(application.state_real == ABERTO){
          gate_cmd(FECHAR);
      }else if(application.state_real == FECHADO){
          gate_cmd(ABRIR);
      }else{
          gate_cmd(ACIONARMOTOR);
      }

      break;
    case LRCMD_STATUS_CENTRAL:
      emberEventControlSetActive(*report_control);
      break;
    case LRCMD_BUTTON:
      if(application.Module_mode == RECEPTOR){
          if(receive->data[0] == 1){
              gate_cmd(ACIONARMOTOR);
          }else if(receive->data[0] == 2){
              sendRadio.cmd = LRCMD_STATUS_CENTRAL;
              sendRadio.data[0] = application.state_real;
              sendRadio.len = 2;
              status = radio_send_packet(&sendRadio, false);
          }else if(receive->data[0] == 4){

          }
      }
      break;
    case LRCMD_SEND_KEY:
      break;
  }

  emberEventControlSetInactive(*radio_control);
}

EmberStatus radio_send_packet(packet_void_t *pck, bool retrying){
  uint8_t buffer_send[8];
  EmberStatus status;
  uint8_t ID_node;

  if(application.Module_mode == ALARM){
      ID_node = 0;
  }else if(application.Module_mode == RECEPTOR){
      ID_node = 1;
  }

  if(!retrying){
      Queue_manager(pck);
  }

  buffer_send[0] = pck->cmd;
  for(uint8_t i = 0; i < (pck->len-1); i++){
      buffer_send[i+1] = pck->data[i];
  }

  if(pck->cmd == LRCMD_JOINED_NETWORK_GATE){
      tx_options = EMBER_OPTIONS_NONE;
  }else{
      tx_options = EMBER_OPTIONS_SECURITY_ENABLED | EMBER_OPTIONS_ACK_REQUESTED;
  }

  status = radioMessageSend(ID_node,(pck->len),buffer_send);

  return status;
}

void Queue_manager(packet_void_t *pck){

  if(pck->cmd != 0){
      for(int i = 0; i < MAX_QUEUE_PACKETS; i++){
          if(!radioQueue[i].slot_used){
              radioQueue[i].attempts = 0;
              radioQueue[i].slot_used = true;
              radioQueue[i].packet = *pck;
              break;
          }
      }
  }
}

void led_blink(uint8_t led, uint8_t blinks, uint16_t speed){
  led_target = led;
  blink_target = blinks;
  ok_to_blink = false;

  sl_sleeptimer_start_periodic_timer_ms(&periodic_timer,speed,led_handler, NULL,0,SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
}

void led_handler(sl_sleeptimer_timer_handle_t *handle, void *data){

  (void)&handle;
  (void)&data;

  switch (led_target) {
    case VERMELHO:

      if((blink_count < (blink_target * 2))){
          blink_count++;
          hGpio_ledToggle(&sl_led_red, true);
      }else{
          joining = false;
          blink_count = 0;
          hGpio_ledTurnOff(&sl_led_red);
          ok_to_blink = true;
          sl_sleeptimer_stop_timer(&periodic_timer);
      }
      break;
    case VERDE:

          if((blink_count < (blink_target * 2))){
              blink_count++;
              hGpio_ledToggle(&sl_led_green, true);
          }else{
              joining = false;
              blink_count = 0;
              hGpio_ledTurnOff(&sl_led_green);
              ok_to_blink = true;
              sl_sleeptimer_stop_timer(&periodic_timer);
          }
          break;
    case AZUL:

          if((blink_count < (blink_target * 2))){
              blink_count++;
              hGpio_ledToggle(&sl_led_blue, true);
          }else{
              joining = false;
              blink_count = 0;
              hGpio_ledTurnOff(&sl_led_blue);
              ok_to_blink = true;
              sl_sleeptimer_stop_timer(&periodic_timer);
          }
          break;
    default:
      break;
  }

}
