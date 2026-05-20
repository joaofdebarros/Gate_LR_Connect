/*
 * Connect.c
 *
 *  Created on: 11 de set. de 2025
 *      Author: joao.victor
 */

#include "Connect.h"
#include "Application/application.h"
/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
ConnectLR_t ConnectLR;
extern UARTDRV_Handle_t sl_uartdrv_usart_central_handle;

#ifndef LED_INSTANCE_0
#define LED_INSTANCE_0      sl_led_led0
#endif

#ifndef LED_INSTANCE_1
#define LED_INSTANCE_1      sl_led_led1
#endif

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
uint8_t data_received[5];

uint8_t packet_received_timeout = 0;

void rx_done_callback(UARTDRV_Handle_t handle,
                       Ecode_t          status,
                       uint8_t         *data,
                       UARTDRV_Count_t  len);
/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/



/***************************************************************************//**
 * Initialize example.
 ******************************************************************************/
EmberEventControl *TimeoutStatus_control;

void connect_init(void)
{
//  USART0->CTRL |= USART_CTRL_RXINV;  // INVERTER RX PARA CIRCUITO COM TRANSISTORES

  USTIMER_Init();

  UARTDRV_Receive(sl_uartdrv_usart_central_handle,
                    &ConnectLR.byte_receive, 1, rx_done_callback);

  emberAfAllocateEvent(&TimeoutStatus_control, &TimeoutStatus_handler);
  emberEventControlSetDelayMS(*TimeoutStatus_control,300);
}


void TimeoutStatus_handler(void){
  GPIO_PinOutSet(gpioPortA, 5);

  connect_get_status();

  if(!application.device_info.device_type_identified){
      packet_received_timeout++;
      if(packet_received_timeout >= 15){
          application.Gate_method = RX;
      }
  }else{
      packet_received_timeout = 0;
  }

  emberEventControlSetDelayMS(*TimeoutStatus_control,300);
}


void gate_cmd(uint8_t cmd){
  if(application.Gate_method == PROG){
      uint8_t buffer_size = 10;
      uint8_t Buffer_TX[40];
      uint8_t data[4] = {cmd,0,0,0};
      montar_pacote(Buffer_TX, buffer_size, 0x02, 0x00,'C', data, 4, GATE);

      gate_packet_transmit(Buffer_TX, buffer_size);
      USTIMER_Delay(10000);
  }else if(application.Gate_method == RX){
      GPIO->USARTROUTE[0].ROUTEEN &= ~GPIO_USART_ROUTEEN_TXPEN;
      GPIO->USARTROUTE[0].ROUTEEN &= ~GPIO_USART_ROUTEEN_RXPEN;

      GPIO_PinOutSet(gpioPortA, 5);
      USART_Enable(USART0, usartDisable);

      GPIO_PinModeSet(gpioPortA, 5, gpioModePushPull, 0);
      GPIO_PinModeSet(gpioPortA, 6, gpioModeInput, 0);

      emberEventControlSetDelayMS(*TimeoutStatus_control,200);
  }
}

void cerca_cmd(cerca_cmd_t cmd){
  uint8_t Buffer_TX[40];
  uint8_t buffer_size;
  buffer_size = 8;
  montar_pacote(Buffer_TX, buffer_size, 0x00, 0x00,cmd, 0X00, 0, CERCA);

  gate_packet_transmit(Buffer_TX, buffer_size);

  USTIMER_Delay(10000);
}

void connect_get_status(){
  if(application.Gate_method == PROG){
      if(application.device_info.device_type == GATE){
          uint8_t buffer_size = 10;
          uint8_t Buffer_TX[40];
          buffer_size = 6;
          montar_pacote(Buffer_TX, buffer_size, 0x02, 0x00,'H', 0X00, 0, GATE);

          gate_packet_transmit(Buffer_TX, buffer_size);

          USTIMER_Delay(10000);

      }else if(application.device_info.device_type == CERCA){
          uint8_t Buffer_TX[40];
          uint8_t buffer_size;
          buffer_size = 8;
          montar_pacote(Buffer_TX, buffer_size, 0x00, 0x00,LIBERA_LED_WIFI, 0X00, 0, CERCA);

          gate_packet_transmit(Buffer_TX, buffer_size);

          USTIMER_Delay(10000);
      }
  }else if(application.Gate_method == RX){
      get_state_gate(&application.state_gate);
  }

  if(application.device_info.device_type_identified == false){
      if(application.device_info.device_type == CERCA){
          application.device_info.device_type = GATE;
      }else{
          application.device_info.device_type = CERCA;
      }
  }
}

void get_state_gate(uint8_t *state){
  if(application.Gate_method == RX){
      *state = GPIO_PinInGet(gpioPortC, 1);
  }else if(application.Gate_method == PROG){
      *state = ConnectLR.gate_info.state;
  }
}

void packet_receive(uint8_t *byte_receive){
  uint8_t lenght;
  uint8_t id;
  uint8_t function;

  switch (ConnectLR.state) {
  case START_ST:
    if (*byte_receive == 0x7E) {

        ConnectLR.state = SIZE_ST;
    }
    UARTDRV_Receive(sl_uartdrv_usart_central_handle,
                        &ConnectLR.byte_receive, 1, rx_done_callback);

    break;
  case SIZE_ST:
    lenght = *byte_receive;
    ConnectLR.data[0] = lenght;
    ConnectLR.state = IDT_ST;
    UARTDRV_Receive(sl_uartdrv_usart_central_handle,
                        &ConnectLR.byte_receive, 1, rx_done_callback);

    break;

  case IDT_ST:
    id = *byte_receive;

    ConnectLR.data[1] = id;
    ConnectLR.state = FUNCTION_ST;

    if(!application.device_info.device_type_identified){
        if(id == NAO_PROGRAMAVEL || id == TRIFLEX_IND || id == TRIFLEX_CONNECT || id == TRIFLEX_IND || id == LOGIC_DOOR || id == FACILITY || id == 17){
            application.device_info.device_type = GATE;
            application.device_info.device_type_identified = true;
            application.Gate_method = PROG;

            memory_write(DEVICE_TYPE_MEMORY_KEY, &application.device_info, sizeof(application.device_info));
        }else{
            application.device_info.device_type = CERCA;
            application.device_info.device_type_identified = true;

            memory_write(DEVICE_TYPE_MEMORY_KEY, &application.device_info, sizeof(application.device_info));
        }
    }

    UARTDRV_Receive(sl_uartdrv_usart_central_handle,
                          &ConnectLR.byte_receive, 1 , rx_done_callback);

    break;

  case FUNCTION_ST:
      function = *byte_receive;
      ConnectLR.data[2] = function;
      ConnectLR.state = DATA_ST;
      UARTDRV_Receive(sl_uartdrv_usart_central_handle,
                          &ConnectLR.data[3], (ConnectLR.data[0] - 4), rx_done_callback);

      break;

  case DATA_ST:
    ConnectLR.state = START_ST;

    if(application.device_info.device_type == GATE){
        ConnectLR.packetError_Gate = gate_packet_demount(ConnectLR.data, ConnectLR.data[0], &ConnectLR.gate_packet);

        if (ConnectLR.packetError_Gate == GATE_PACKET_OK) {
            application.Connect_ID = ConnectLR.gate_packet.id;
            application.Gate_method = PROG;
            emberEventControlSetDelayMS(*TimeoutStatus_control,300);
            if(ConnectLR.gate_packet.function == 'H'){
                ConnectLR.gate_info.state = ConnectLR.gate_packet.data[5];
                switch (ConnectLR.gate_info.state) {
                  case ABERTO:
                    app_log_info("Aberto");

                    break;
                  case ABRINDO:
                    app_log_info("Abrindo");

                    break;
                  case FECHADO:
                    app_log_info("fechado");

                    break;
                  case FECHANDO:
                    app_log_info("Fechando");

                    break;
                  case SEMIABERTO:
                    app_log_info("Semiaberto");

                    break;

                  default:
                    break;
                }

            }else if(ConnectLR.gate_packet.function == 'C'){
                ConnectLR.gate_info.action = ConnectLR.gate_packet.data[0];
            }
        }
    }else if(application.device_info.device_type == CERCA){
        ConnectLR.packetError_Cerca = cerca_packet_demount(ConnectLR.data, ConnectLR.data[0], &ConnectLR.cerca_packet);

        if (ConnectLR.packetError_Cerca == CERCA_PACKET_OK) {
            application.Connect_ID = ConnectLR.cerca_packet.id;
            application.Status_cerca.Status1.byte = ConnectLR.cerca_packet.Status_1;
            application.Status_cerca.Status2.byte = ConnectLR.cerca_packet.Status_2;
            application.Status_cerca.Status3.byte = ConnectLR.cerca_packet.Status_3;

            application.All_status_cerca.Status3_cerca_bits.AC = application.Status_cerca.Status3.Status3_cerca_bits.AC;
            application.All_status_cerca.Status3_cerca_bits.Choque = application.Status_cerca.Status1.Status1_cerca_bits.Choque;
            application.All_status_cerca.Status3_cerca_bits.PGM = application.Status_cerca.Status1.Status1_cerca_bits.PGM;
            application.All_status_cerca.Status3_cerca_bits.Panico = application.Status_cerca.Status1.Status1_cerca_bits.Panico;
            application.All_status_cerca.Status3_cerca_bits.Retorno = application.Status_cerca.Status1.Status1_cerca_bits.Retorno;
            application.All_status_cerca.Status3_cerca_bits.STTS = application.Status_cerca.Status3.Status3_cerca_bits.STTS;
            application.All_status_cerca.Status3_cerca_bits.Setor = application.Status_cerca.Status1.Status1_cerca_bits.Setor;
//            application.All_status_cerca.Status3_cerca_bits.Sirene = application.Status_cerca.Status1.Status1_cerca_bits.Sirene;
            application.All_status_cerca.Status3_cerca_bits.Sirene = 1;
        }
    }

    UARTDRV_Receive(sl_uartdrv_usart_central_handle,
                        &ConnectLR.byte_receive, 1, rx_done_callback);
    break;

  default:
  }
}

void gate_packet_transmit(uint8_t *byte_transmit,uint8_t len){
  if(application.Gate_method == PROG){
      UARTDRV_Transmit(sl_uartdrv_usart_central_handle, byte_transmit, len, NULL);
      USTIMER_Delay(1000);
  }
}

void rx_done_callback(UARTDRV_Handle_t handle,
                             Ecode_t   status,
                             uint8_t *data,
                             UARTDRV_Count_t  len)
{
  (void) handle;
  (void) status;
  (void) data;
  (void) len;

  packet_receive(&ConnectLR.byte_receive);
}
