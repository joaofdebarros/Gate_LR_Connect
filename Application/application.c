/*
 * application.c
 *
 *  Created on: 12 de ago. de 2024
 *      Author: diego.marinho
 */

#include "application.h"

application_t application;

EmberEventControl *radio_control;
EmberEventControl *CheckState_control;

void app_init(void){
  app_log_info("Teste init");
  connect_init();
  emberAfAllocateEvent(&radio_control, &radio_handler);
  emberAfAllocateEvent(&CheckState_control, &CheckState_handler);
  emberEventControlSetDelayMS(*CheckState_control,2000);
  get_state(&application.state_before);
}

void CheckState_handler(void){
  EmberStatus status;
  get_state(&application.state_real);
  if(application.state_real != application.state_before){
      application.state_before = application.state_real;
      app_log_info("Change status");
      packet_void_t SendChange;
      SendChange.cmd = CHANGE_STATUS;
      SendChange.data[0] = application.state_before;
      SendChange.len = 2;
      status = radio_send_packet(&SendChange);
      if(status == EMBER_SUCCESS){
          sl_led_toggle(&sl_led_led1);
      }
  }
  emberEventControlSetDelayMS(*CheckState_control,300);
}

void radio_handler(void){
  volatile packet_void_t *receive;

  receive = &application.radio.Packet;

  switch(receive->cmd){
    case MOTOR_CONTROL:
      gate_cmd(ACIONARMOTOR);
      break;
  }
  emberEventControlSetInactive(*radio_control);
}

EmberStatus radio_send_packet(packet_void_t *pck){
  uint8_t buffer_send[8];
  EmberStatus status;

  buffer_send[0] = pck->cmd;
  for(uint8_t i = 0; i < (pck->len-1); i++){
      buffer_send[i+1] = pck->data[i];
  }
  status = radioMessageSend(0,pck->len,buffer_send);

  return status;
}




