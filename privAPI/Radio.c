/*
 * Radio.c
 *
 *  Created on: 13 de ago. de 2024
 *      Author: diego.marinho
 */

#include "Radio.h"

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
  app_log_info("Teste incoming");
  privcallback_Radio_Receive(message->payload,message->length);
  //callback_Radio_Receive(message->payload,message->length);
//  if (message->endpoint == SL_SENSOR_SINK_ENDPOINT) {
//    app_log_info("RX: Data from 0x%04X:", message->source);
//    for (uint8_t i = SL_SENSOR_SINK_DATA_OFFSET; i < message->length; i++) {
//      app_log_info(" %x", message->payload[i]);
//    }
//    app_log_info("\n");
//  }
}
