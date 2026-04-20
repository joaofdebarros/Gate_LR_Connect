/*
 * Radio.c
 *
 *  Created on: 13 de ago. de 2024
 *      Author: diego.marinho
 */

#include "Radio.h"
#include "Application/application.h"

extern EmberMessageOptions tx_options;

extern EmberKeyData connect_network_key;
extern bool initialized;
extern EmberEventControl *report_control;

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
  if(message->payload[0] == LRCMD_SEND_KEY && application.Status_Operation == WAIT_REGISTRATION){
        memcpy(connect_network_key.contents,message->payload + 1,EMBER_ENCRYPTION_KEY_SIZE);

        if (set_security_key(connect_network_key.contents, (size_t)EMBER_ENCRYPTION_KEY_SIZE) == true) {
            memory_write(SECURITY_KEY_MEMORY_KEY, connect_network_key.contents, sizeof(connect_network_key.contents));
            if(application.Status_Operation == WAIT_REGISTRATION && initialized){
                emberEventControlSetDelayMS(*report_control, 1000);
            }
  //          app_log_info("Connect: set the random key as network key succeed\n");
        } else {
  //          app_log_error("Connect: set the random key as network key failed\n");
        }
    }else{
        if(application.Status_Operation == WAIT_REGISTRATION){
            privcallback_Radio_Receive(message->payload,message->length);
            application.radio.RSSI = -(message->rssi);
        }else if(application.Status_Operation == PERIOD_INSTALATION || application.Status_Operation == OPERATION_MODE){
            privcallback_Radio_Receive(message->payload,message->length);
            application.radio.RSSI = -(message->rssi);
        }
    }
}
