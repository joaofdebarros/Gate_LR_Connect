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

packet_void_t sendRadio;
send_queue_t radioQueue[MAX_QUEUE_PACKETS];

void app_init(void){
  app_log_info("Teste init");
  connect_init();

  emberEventControlSetDelayMS(*CheckState_control,2000);
  get_state(&application.state_before);
}

void CheckState_handler(void){
  EmberStatus status;
  get_state(&application.state_real);
  if(application.state_real != application.state_before){
      application.state_before = application.state_real;
      app_log_info("Change status");
      sendRadio.cmd = CHANGE_STATUS;
      sendRadio.data[0] = application.state_before;
      sendRadio.len = 2;
      status = radio_send_packet(&sendRadio, false);
      if(status == EMBER_SUCCESS){
          sl_led_toggle(&sl_led_led1);
      }
  }
  emberEventControlSetDelayMS(*CheckState_control,300);
}

void radio_handler(void){
  volatile packet_void_t *receive;

  receive = &application.radio.Packet;

  application.radio.LastCMD = receive->cmd;

  switch(receive->cmd){
    case MOTOR_CONTROL:
      gate_cmd(ACIONARMOTOR);
      break;
    case STATUS_CENTRAL:
      emberEventControlSetActive(*report_control);
      break;
  }
  emberEventControlSetInactive(*radio_control);
}

EmberStatus radio_send_packet(packet_void_t *pck, bool retrying){
  uint8_t buffer_send[8];
  EmberStatus status;

  if(!retrying){
      Queue_manager(pck);
  }

  buffer_send[0] = pck->cmd;
  for(uint8_t i = 0; i < (pck->len-1); i++){
      buffer_send[i+1] = pck->data[i];
  }
  status = radioMessageSend(0,pck->len,buffer_send);

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




