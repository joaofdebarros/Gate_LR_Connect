/*
 * callbacks.c
 *
 *  Created on: 18 de july de 2024
 *      Author: diego.marinho
 */


#include "callbacks.h"


packet_void_t Packet;
extern application_t application;
extern EmberEventControl *radio_control;
extern EmberEventControl *TimeoutStatus_control;


/*
 * Globas and Externs
 */



void callback_Radio_Receive(uint8_t *data, uint8_t len){
  packet_data_demount(data,len,&application.radio.Packet);
//  emberEventControlSetInactive(*TimeoutStatus_control);
//  emberEventControlSetDelayMS(*TimeoutStatus_control,5000);
//  emberEventControlSetDelayMS(*radio_control,100);
  emberEventControlSetActive(*radio_control);
}






