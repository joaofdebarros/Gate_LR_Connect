/*
 * Radio.c
 *
 *  Created on: 13 de ago. de 2024
 *      Author: diego.marinho
 */

#include "Radio.h"
#include "Application/application.h"

extern EmberMessageOptions tx_options;

void SL_WEAK privcallback_Radio_Receive(uint8_t *data,uint8_t length);

status_radio_t radioMessageSend(uint8_t destination, uint8_t messageLength, uint8_t *data){
  EmberStatus status;
  status = emberMessageSend(destination,1,0,messageLength,data,tx_options);
  return status;
}

/**************************************************************************//**
 * This function is called when a message is received.
 *****************************************************************************/
void emberAfIncomingMessageCallback(EmberIncomingMessage *message)
{
  if(application.Status_Operation == WAIT_REGISTRATION){
      privcallback_Radio_Receive(message->payload,message->length);
      application.radio.RSSI = -(message->rssi);
  }else if(application.Status_Operation == PERIOD_INSTALATION || application.Status_Operation == OPERATION_MODE){
      uint16_t received_key = 0;

      received_key = (message->payload[message->length - 1] << 8) | (message->payload[message->length - 2]);

      if(application.LR_key != 0){
          if(received_key == application.LR_key){
              privcallback_Radio_Receive(message->payload,message->length - 2);
              application.radio.RSSI = -(message->rssi);
          }
      }else{
          privcallback_Radio_Receive(message->payload,message->length - 2);
          application.radio.RSSI = -(message->rssi);
      }
  }
}
