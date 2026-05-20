/*
 * Connect.h
 *
 *  Created on: 11 de set. de 2025
 *      Author: joao.victor
 */

#ifndef API_CONNECT_CONNECT_H_
#define API_CONNECT_CONNECT_H_

//#include "API/Button/simple_button_baremetal.h"
#include "sl_simple_led_instances.h"
#include "uartdrv.h"
#include "sl_uartdrv_instances.h"
#include <string.h>
#include "sl_hal_timer.h"
#include "ustimer.h"
#include "ember-types.h"
#include "app_framework_common.h"
#include "app_log.h"


#define PACKET_HEADER_LEN 1
#define PACKET_LENGTH_LEN 1
#define PACKET_FUNCTION_LEN 1
#define PACKET_CHECKSUM_LEN 1
#define PACKET_TAIL_LEN 1

#define start_byte 0x7E
#define stop_byte 0x81

typedef enum{
  NAO_PROGRAMAVEL,
  TRIFLEX_IND,
  TRIFLEX_CONNECT,
  LOGIC_DOOR,
  FACILITY = 16, //16 e 17
  CERCA_20_22K = 68,
  CERCA_CR2S = 69,
  CERCA_18K = 70,
  CERCA_battery = 71,
  CERCA_economy = 72,
  CERCA_power = 73,
  CERCA_CR2S_smartOn = 74,
  CERCA_CR2S_smartOn_22k = 75,
  Cerca_Impacto_10K = 76,
  Cerca_Impacto_10K_WiFi = 77
} Connect_ID_e;

typedef enum{
  ABERTO = 1,
  ABRINDO,
  FECHADO,
  FECHANDO,
  SEMIABERTO,
  TRAVADO,
  LENDO_ABRE,
  LENDO_FECHA,
  INICIAL,
  ERROR = 0xFF,
} gate_status_t;

typedef enum{
  ABRIR = 1,
  FECHAR,
  TRAVAR,
  PARAR,
  LG = 6,
  ACIONARMOTOR = 12,
} gate_cmd_t;

typedef enum{
  ARMA_CHOQUE = 1,
  DESARMA_CHOQUE,
  ARMA_SETOR,
  DESARMA_SETOR,
  LIGAR_PANICO,
  DESLIGAR_PANICO,
  LIBERA_LED_WIFI = 14,
  LIGA_PGM = 17,
  DESLIGA_PGM = 18,
  STTS_ON = 21,
  STTS_OFF = 22
} cerca_cmd_t;

typedef struct{
  uint8_t state;
  uint8_t action;
} gate_info_t;

typedef struct {
  uint8_t len;
  uint8_t id;
  uint8_t function;
  uint8_t data[40];
  uint8_t checksum;
  uint8_t tail;
} gate_packet_t;

typedef struct {
  uint8_t len;
  uint8_t id;
  uint8_t Status_1;
  uint8_t Status_2;
  uint8_t Status_3;
  uint8_t tx[4];
  uint8_t wifi;
  uint8_t checksum;
  uint8_t tail;
} cerca_packet_t;

typedef enum {
  START_ST = 0,
  SIZE_ST,
  IDT_ST,
  FUNCTION_ST,
  DATA_ST,
} state_receive_t;

typedef enum {
  GATE_PACKET_OK,
  GATE_PACKET_FAIL_HEADER,
  GATE_PACKET_FAIL_TAIL,
  GATE_PACKET_FAIL_LENGHT,
  GATE_PACKET_FAIL_CHECKSUM,
  GATE_PACKET_FAIL_UNKNOWN = 0xFF
} packet_errorGATE_e;

typedef enum {
  CERCA_PACKET_OK,
  CERCA_PACKET_FAIL_HEADER,
  CERCA_PACKET_FAIL_TAIL,
  CERCA_PACKET_FAIL_LENGHT,
  CERCA_PACKET_FAIL_CHECKSUM,
  CERCA_PACKET_FAIL_UNKNOWN = 0xFF
} packet_errorCERCA_e;

typedef struct {
  packet_errorGATE_e packetError_Gate;
  packet_errorCERCA_e packetError_Cerca;
  gate_packet_t gate_packet;
  cerca_packet_t  cerca_packet;
  gate_info_t gate_info;
  uint8_t byte_receive;
  uint8_t data[40];
  state_receive_t state;
} ConnectLR_t;

void connect_init(void);
void TimeoutStatus_handler(void);
void Timeout_Connect_handler(void);

void get_state_gate(uint8_t *state);
void gate_packet_transmit(uint8_t *byte_transmit,uint8_t len);
void gate_cmd(uint8_t cmd);
void cerca_cmd(cerca_cmd_t cmd);
void connect_get_status();
#endif /* API_CONNECT_CONNECT_H_ */
