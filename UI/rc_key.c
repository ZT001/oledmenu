#include "board.h"
#include "fsl_pint.h"
#include "rc_delay.h"
#include "rc_Key.h"


#define KEY1_GPIO_PIN     19	/* GPIO pin number mapped to PININT */
#define KEY2_GPIO_PIN     30	/* GPIO pin number mapped to PININT */ 
#define KEY_GPIO_PORT     0	/* GPIO port number mapped to PININT */

//#define GPIO_PININT_INDEX0   PININTSELECT0	/* PININT index used for GPIO mapping */
//#define PININT_IRQ_HANDLER0  PIN_INT0_IRQHandler	/* GPIO interrupt IRQ function name */
//#define PININT_NVIC_NAME0    PIN_INT0_IRQn	/* GPIO interrupt NVIC interrupt name */

//#define GPIO_PININT_INDEX1   PININTSELECT1	/* PININT index used for GPIO mapping */
//#define PININT_IRQ_HANDLER1  PIN_INT1_IRQHandler	/* GPIO interrupt IRQ function name */
//#define PININT_NVIC_NAME1    PIN_INT1_IRQn	/* GPIO interrupt NVIC interrupt name */  




static uint32_t _Key1Status;
static uint32_t _Key2Status;
static uint32_t _Key1StartTime;
static uint32_t _Key2StartTime;
static uint32_t _TimeNow;
uint32_t _Key1HoldingTime;
uint32_t _Key2HoldingTime;
ekey_status _KeyStatus;

static BaseType_t _xHigherPriorityTaskWoken = pdFALSE;
static SemaphoreHandle_t _KeySemaphore;

void GetKeyStatus(void)
{
	/*默认为触发中断按键*/
	//_KeyStatus=keystatus;
	if(_Key1HoldingTime>0 && _Key2HoldingTime==0)//key 1
	{
		_KeyStatus=KEY_STATUS_SET;
	}
	else if(_Key1HoldingTime==0 && _Key2HoldingTime>0)//key 2
	{
		_KeyStatus=KEY_STATUS_CIRCLE;
	}
	else if(_Key1HoldingTime>0 && _Key2HoldingTime>0)// key 1,2
	{
		_KeyStatus=KEY_STATUS_ALL;
	}
}

/*key2 中断*/
void PININT_IRQ_HANDLER0(pint_pin_int_t pintr, uint32_t pmatch_status)
{  
	 PINT_PinInterruptClrStatus(PINT,kPINT_PinInt0);

	_Key1Status =GPIO_PinRead(GPIO, 0, KEY1_GPIO_PIN);
	if(_Key1Status==0)
	{
		_Key1StartTime=xTaskGetTickCountFromISR();
	}
	else
	{
		_TimeNow=xTaskGetTickCountFromISR();
		_Key1HoldingTime=(_TimeNow-_Key1StartTime)*portTICK_PERIOD_MS;
		_Key1StartTime=0;
		GetKeyStatus();
	}
	xSemaphoreGiveFromISR(_KeySemaphore,&_xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(_xHigherPriorityTaskWoken);
}
//KEY_1 中断
void PININT_IRQ_HANDLER1(pint_pin_int_t pintr, uint32_t pmatch_status)
{
	PINT_PinInterruptClrStatus(PINT,kPINT_PinInt1);
	
	_Key2Status =GPIO_PinRead(GPIO, KEY_GPIO_PORT, KEY2_GPIO_PIN);
	if(_Key2Status==0)
	{
		_Key2StartTime=xTaskGetTickCountFromISR();
	}
	else
	{
		_TimeNow=xTaskGetTickCountFromISR();
		_Key2HoldingTime=(_TimeNow-_Key2StartTime)*portTICK_PERIOD_MS;
		_Key2StartTime=0;
		GetKeyStatus();		
	}
	xSemaphoreGiveFromISR(_KeySemaphore,&_xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(_xHigherPriorityTaskWoken);
}

/*初始化key*/
void Interruppt_Init(void)
{
	NVIC_SetPriority(PIN_INT0_IRQn,4);
	NVIC_SetPriority(PIN_INT1_IRQn,4);
	NVIC_EnableIRQ(PIN_INT0_IRQn);
	NVIC_EnableIRQ(PIN_INT1_IRQn);
}

/*初始化key*/
void KeyInit(SemaphoreHandle_t keySemaphore)
{
	//init key
	Interruppt_Init();
	_KeySemaphore=keySemaphore;
}
/*等待按键触发状态*/
EventBits_t KeyWait(void)
{
   return xSemaphoreTake(_KeySemaphore, portMAX_DELAY);
}
/*重置案件状态*/
void KeyReset(void)
{
	_KeyStatus=KEY_STATUS_NO_KEY_PRESS;
	_Key1StartTime=0;
	_Key2StartTime=0;
	_Key1HoldingTime=0;
	_Key2HoldingTime=0;
}
/*扫描按键（双键）*/
void KeyScan(ekey_status* keystatus,uint32_t* key1time,uint32_t* key2time)
{
	bool KeyVal1=0,KeyVal2=0;
	uint32_t key1cc=0,key2cc=0,tempcc=0;
	
	KeyVal1 =GPIO_PinRead(GPIO, KEY_GPIO_PORT, KEY1_GPIO_PIN);
	KeyVal2 = GPIO_PinRead(GPIO, KEY_GPIO_PORT,KEY2_GPIO_PIN);
	//没有按键按下则返回
	if(KeyVal1!=0 && KeyVal2!=0)
	{
		*keystatus= KEY_STATUS_NO_KEY_PRESS;return;
	}	
	//
	tempcc=xTaskGetTickCount();
	//等待按键释放
	while(!KeyVal1 || !KeyVal2) 
	{
		KeyVal1 = GPIO_PinRead(GPIO, KEY_GPIO_PORT,KEY1_GPIO_PIN);
		KeyVal2 = GPIO_PinRead(GPIO, KEY_GPIO_PORT,KEY2_GPIO_PIN);
			
		key1cc+=(KeyVal1==0? (xTaskGetTickCount()-tempcc):0);
		key2cc+=(KeyVal2==0? (xTaskGetTickCount()-tempcc):0);
		//refresh
		tempcc=xTaskGetTickCount();
		delay_ms(10);
	}
	//get status
	if(key1cc>0 && key2cc==0)//key 1
	{
		*keystatus=KEY_STATUS_SET;
	}
	else if(key1cc==0 && key2cc>0)//key 2
	{
		*keystatus=KEY_STATUS_CIRCLE;
	}
	else if(key1cc>0 && key2cc>0)// key 1,2
	{
		*keystatus=KEY_STATUS_ALL;
	}
	
	//循环结束，赋值
	*key1time= key1cc*portTICK_PERIOD_MS ;//ms 计时器10ms一次计数
	*key2time= key2cc*portTICK_PERIOD_MS;//ms

}
