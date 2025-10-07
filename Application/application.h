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

typedef enum{
  SENSOR_IDLE = 0,
  SENSOR_CONFIGURANDO,
  SENSOR_CONFIGURADO
}Status_Sensor_t;

typedef enum{
  WAIT_REGISTRATION = 0,
  PERIOD_INSTALATION,
  OPERATION_MODE
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
}application_radio_t;



typedef struct{
  application_radio_t radio;
  Status_Operation_t Status_Operation;
  Status_Central_t Status_Central;
  gate_status_t state_real;
  gate_status_t state_before;
}application_t;

extern application_t application;

/*
 * Prototypes
 */
void app_init(void);
EmberStatus radio_send_packet(packet_void_t *pck);
void CheckState_handler(void);
void radio_handler(void);
void timeout_handler(void);
void motionDetected_handler(void);
void PeriodInstalation_handler(void);
void TimeoutAck_handler(void);

#endif /* APPLICATION_APPLICATION_H_ */
