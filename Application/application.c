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

extern bool joining;

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

          sendRadio.cmd = CHANGE_STATUS;
          sendRadio.len = 3;
          sendRadio.data[0] = application.state_before;
          sendRadio.data[1] = application.radio.RSSI;

          status = radio_send_packet(&sendRadio, false);
      }else if(application.Module_mode == RECEPTOR){
          sendRadio.cmd = CHANGE_STATUS;
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
    case MOTOR_CONTROL:
      if(application.state_real == ABERTO){
          gate_cmd(FECHAR);
      }else if(application.state_real == FECHADO){
          gate_cmd(ABRIR);
      }else{
          gate_cmd(ACIONARMOTOR);
      }

      break;
    case STATUS_CENTRAL:
      emberEventControlSetActive(*report_control);
      break;
    case TX_CMD_BT:
      if(application.Module_mode == RECEPTOR){
          if(receive->data[0] == 1){
              gate_cmd(ACIONARMOTOR);
          }else if(receive->data[0] == 2){
              sendRadio.cmd = STATUS_GATE;
              sendRadio.data[0] = application.state_real;
              sendRadio.len = 2;
              status = radio_send_packet(&sendRadio, false);
          }else if(receive->data[0] == 4){

          }
      }
      break;
    case LR_KEY:
      application.LR_key = (receive->data[1] << 8) | (receive->data[0]);

      memory_write(LR_KEY_MEMORY_KEY, &application.LR_key, sizeof(application.LR_key));
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
  buffer_send[pck->len] = application.LR_key;
  buffer_send[(pck->len)+1] = application.LR_key >> 8;

  status = radioMessageSend(ID_node,(pck->len)+2,buffer_send);

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

  sl_sleeptimer_start_periodic_timer_ms(&periodic_timer,speed,led_handler, NULL,0,SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
}

void led_handler(sl_sleeptimer_timer_handle_t *handle, void *data){

  (void)&handle;
  (void)&data;

  switch (led_target) {
    case VERMELHO:

      if((blink_count < (blink_target * 2))){
          blink_count++;
          hGpio_ledToggle(&sl_led_led0, true);
      }else{
          joining = false;
          blink_count = 0;
          hGpio_ledTurnOff(&sl_led_led0, true);
          sl_sleeptimer_stop_timer(&periodic_timer);
      }
      break;
    default:
      break;
  }

}
