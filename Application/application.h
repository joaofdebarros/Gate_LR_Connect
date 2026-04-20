/*
 * application.h
 *
 *  Created on: 12 de ago. de 2024
 *      Author: diego.marinho
 */

#ifndef APPLICATION_APPLICATION_H_
#define APPLICATION_APPLICATION_H_

#include "API/packet/packet.h"
#include "privAPI/Radio.h"
#include "app_framework_common.h"
#include "ember-types.h"
#include "app_log.h"
#include "API/Connect/Connect.h"
#include "API/memory/memory.h"
#include "boards/chipset/efr32.h"

#define MAX_QUEUE_PACKETS 3
#define MAX_QUEUE_PACKET_ATTEMPTS 10

#define SLOW_SPEED_BLINK  1000
#define MED_SPEED_BLINK   200
#define FAST_SPEED_BLINK  100

typedef enum{
  SENSOR_IDLE = 0,
  SENSOR_CONFIGURANDO,
  SENSOR_CONFIGURADO
}Status_Sensor_t;

typedef enum{
  WAIT_REGISTRATION = 0,
  PERIOD_INSTALATION,
  OPERATION_MODE,
}Status_Operation_t;

typedef enum{
  CONTINUOUS = 0,
  ECONOMIC,
  UECONOMIC
}Mode_Operation_t;

typedef enum{
  CONTROL = 0,
  MOTION_DETECT,
  OPEN_CLOSE_DETECT,
  GATE
}Type;

typedef enum{
  ARMED = 0,
  DISARMED
}Status_Central_t;

typedef enum{
  LONG_RANGE = 0,
  MID_RANGE
}Range_t;

typedef union
{
    uint8_t Statusbyte;

    struct
    {
        uint8_t operation             :2;
        uint8_t statusCentral         :1;
        uint8_t reserved              :5;
    } Status;

}
SensorStatus_t;

typedef union
{
    uint8_t Registerbyte;

    struct
    {
        uint8_t Type                  :5;
        uint8_t range                 :2;
        uint8_t reserved              :1;
    } Status;

}
Register_Sensor_t;

typedef struct{
  packet_void_t Packet;
  uint8_t LastCMD;
  uint8_t RSSI;
}application_radio_t;

typedef enum{
  ALARM,
  RECEPTOR
}Module_mode_t;

typedef enum{
  PROG,
  RX
}Gate_mode_t;

typedef enum{
  VERMELHO = 0,
  VERDE,
  AZUL
}LED_t;

typedef struct{
  application_radio_t radio;
  Status_Operation_t Status_Operation;
  Module_mode_t Module_mode;
  Gate_mode_t Gate_mode;
  Status_Central_t Status_Central;
  gate_status_t state_real;
  gate_status_t state_before;
}application_t;

extern application_t application;

/*
 * Prototypes
 */
void app_init(void);
EmberStatus radio_send_packet(packet_void_t *pck, bool retrying);
void CheckState_handler(void);
void radio_handler(void);
void timeout_handler(void);
void led_blink(uint8_t led, uint8_t blinks, uint16_t speed);
void led_handler(sl_sleeptimer_timer_handle_t *handle, void *data);
void TimeoutAck_handler(void);
void Queue_manager(packet_void_t *pck);
#endif /* APPLICATION_APPLICATION_H_ */
