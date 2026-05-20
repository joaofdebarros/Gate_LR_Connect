/*
 * packet.c
 *
 *  Created on: 9 de ago de 2024
 *      Author: diego.marinho
 */

#include "packet.h"


/*
 * Globals
 */




/*
 * Macros
 */


/*
 * Privates
 */






/*
 * Publics
 */

uint8_t montar_pacote(uint8_t *tx, uint8_t size, uint8_t id, uint8_t adrs,
                      uint8_t fnct, uint8_t *data, uint8_t payload_size,
                      uint8_t device_type) {
  uint8_t total_size = 0;

  if (device_type == GATE) {
    total_size = payload_size + 6;

    tx[0] = start_byte;
    tx[1] = size;
    tx[2] = id;
    tx[3] = fnct;

    for (int i = 0; i < payload_size; i++) {
      tx[4 + i] = data[i];
    }

    tx[4 + payload_size] = calculate_checksum(tx, payload_size, device_type);
    tx[5 + payload_size] = stop_byte;
  } else if(device_type == CERCA){
      total_size = payload_size + 5;

      tx[0] = start_byte;
      tx[1] = size;
      tx[2] = 0x4a;
      tx[3] = 0x54;
      tx[4] = fnct;
      tx[5] = 0;
      tx[6] = 0;
      tx[7] = 0x51;
  }else{
      total_size = payload_size + 7;

      tx[0] = start_byte;
      tx[1] = size;
      tx[2] = id;
      tx[3] = adrs;
      tx[4] = fnct;

      for (int i = 0; i < payload_size; i++) {
          tx[5 + i] = data[i];
      }

      tx[5 + payload_size] = calculate_checksum(tx, payload_size, device_type);
      tx[6 + payload_size] = stop_byte;
  }

  return total_size;
}

packet_errorGATE_e gate_packet_demount(uint8_t *datain, uint16_t len,
                                           gate_packet_t *packet) {
  uint8_t checksum_validate;
  uint16_t i, size, lHold;

  if (datain == NULL || packet == NULL) {
    return GATE_PACKET_FAIL_UNKNOWN;
  }
  lHold = (len - 6);
  checksum_validate = 0x7E;
  // Transport all bytes to the struct
  size = 0;
  memset(packet, 0, sizeof(gate_packet_t));

  for (i = 0; i < PACKET_LENGTH_LEN; i++) {
    packet->len = (datain[size++]);
  }

  checksum_validate ^= packet->len;

  for (i = 0; i < PACKET_FUNCTION_LEN; i++) {
    packet->id = datain[size++];
  }
  checksum_validate ^= packet->id;

  for (i = 0; i < PACKET_FUNCTION_LEN; i++) {
    packet->function = datain[size++];
  }
  checksum_validate ^= packet->function;

  if (lHold > sizeof(packet->data)) {
    return GATE_PACKET_FAIL_UNKNOWN;  // pacote inválido
  }

  memcpy(packet->data, &datain[size], lHold);
  size += lHold;

  for (i = 0; i < lHold; i++) {
    checksum_validate ^= packet->data[i];
  }

  for (i = 0; i < PACKET_CHECKSUM_LEN; i++) {
    packet->checksum = (datain[size++]);
  }
  checksum_validate = ~checksum_validate;

  for (i = 0; i < PACKET_TAIL_LEN; i++) {
    packet->tail = (datain[size++]);
  }

  if (checksum_validate != packet->checksum) {
    return GATE_PACKET_FAIL_CHECKSUM;
  }

  return GATE_PACKET_OK;
}

packet_errorCERCA_e cerca_packet_demount(uint8_t *datain, uint16_t len,
                                    cerca_packet_t *packet){
  uint8_t checksum_validate;
  uint16_t i, size, lHold;

  if (datain == NULL || packet == NULL) {
      return CERCA_PACKET_FAIL_UNKNOWN;
  }
  lHold = (len - 5);
  checksum_validate = 0x7E;
  // Transport all bytes to the struct
  size = 0;
  memset(packet, 0, sizeof(cerca_packet_t));

  for (i = 0; i < PACKET_LENGTH_LEN; i++) {
      packet->len = (datain[size++]);
  }

  checksum_validate ^= packet->len;

  for (i = 0; i < PACKET_FUNCTION_LEN; i++) {
      packet->id = datain[size++];
  }
  checksum_validate ^= packet->id;

  if (lHold > (8)) {
      return CERCA_PACKET_FAIL_UNKNOWN;  // pacote inválido
  }

  memcpy(&packet->Status_1, &datain[size], sizeof(packet->Status_1));
  size++;

  memcpy(&packet->Status_2, &datain[size], sizeof(packet->Status_2));
  size++;

  memcpy(&packet->Status_3, &datain[size], sizeof(packet->Status_3));
  size++;

  memcpy(&packet->tx, &datain[size], sizeof(packet->tx));
  size += sizeof(packet->tx);

  memcpy(&packet->wifi, &datain[size], sizeof(packet->wifi));
  size++;

//  for (i = 0; i < lHold; i++) {
//      checksum_validate ^= packet->data[i];
//  }

  for (i = 0; i < PACKET_CHECKSUM_LEN; i++) {
      packet->checksum = (datain[size++]);
  }
  checksum_validate = ~checksum_validate;

  for (i = 0; i < PACKET_TAIL_LEN; i++) {
      packet->tail = (datain[size++]);
  }

//  if (checksum_validate != packet->checksum) {
//      return GATE_PACKET_FAIL_CHECKSUM;
//  }

  return CERCA_PACKET_OK;
}

uint8_t calculate_checksum(uint8_t *buffer, uint8_t payload_size,
                           uint8_t device_type) {
  uint8_t sum = 0;
  uint8_t header_size = 0;

  if (device_type == GATE) {
    header_size = 4;
  } else {
    header_size = 5;
  }

  for (uint8_t i = 0; i < (header_size + payload_size); i++) {
    sum ^= buffer[i];
  }

  return ~sum;
}

packet_error_e packet_data_demount(uint8_t *inData, uint8_t inLen, packet_void_t *packet){
  uint8_t i;
  packet->cmd = inData[0];
  for(i = 0; i < inLen; i++){
      packet->data[i] = inData[i+1];
  }

  return PACKET_OK;
}






