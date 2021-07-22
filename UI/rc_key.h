#ifndef _ZTKEY_H_
#define _ZTKEY_H_
          
#include <stdint.h>
#include "freertos.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

#define KEY_PRESS		0x20
//key status
typedef enum  
{
		KEY_STATUS_NO_KEY_PRESS   =0x01 , //û�а�������
		KEY_STATUS_SET    				=0x02,  //������1����
		KEY_STATUS_CIRCLE 				=0x04,  //������2����
		KEY_STATUS_ALL    				=0x08  //����1��2ͬʱ����
}ekey_status;

extern uint32_t _Key1HoldingTime;
extern uint32_t _Key2HoldingTime;
extern ekey_status _KeyStatus;


void KeyInit(SemaphoreHandle_t keySemaphore);
void KeyScan(ekey_status* keystatus,uint32_t* key1time,uint32_t* key2time);
void KeyReset(void);
EventBits_t KeyWait(void);
#endif

