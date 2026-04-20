/*
 * pckDataStructure.h
 *
 *  Created on: 9 de ago de 2024
 *      Author: diego.marinho
 */

#ifndef ET001_PCKDATASTRUCTURE_H_
#define ET001_PCKDATASTRUCTURE_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * defines
 */

#define PROP_ASYNC				(1 << 0)
#define PROP_DISCONNECT 		(1 << 1)

/*
 * Enumerates
 */
typedef enum SensorCmd_e{
  LRCMD_FORM_NETWORK = 14,
   LRCMD_JOINED_NETWORK = 15,
   LRCMD_BUTTON = 16,
   LRCMD_JOINED_NETWORK_SENSOR = 21,
   LRCMD_DETECTED_SENSOR = 22,
   LRCMD_JOINED_NETWORK_GATE = 28,
   LRCMD_WRITE_CONTROL_GATE = 29,
   LRCMD_GATE_CHANGE_STATUS = 30,
   LRCMD_ADD_PARTITION_SENSOR = 40,
   LRCMD_SETUP_IVP = 41,
   LRCMD_KEEP_ALIVE = 42,
   LRCMD_STATUS_CENTRAL = 43,
   LRCMD_RESET_NETWORK = 44,
   LRCMD_MODULE_CONNECTED = 45,
   LRCMD_GET_KEY = 46,
   LRCMD_SEND_KEY = 47,

	CMD_UNKNOWN = 0xFF
}ModuleLRCmd_e;

typedef struct{
  ModuleLRCmd_e cmd;
		uint8_t data[8];
		uint8_t len;
}packet_void_t;

typedef struct{
    bool slot_used;
    uint8_t attempts;
    packet_void_t packet;
}send_queue_t;

#endif /* ET001_PCKDATASTRUCTURE_H_ */
