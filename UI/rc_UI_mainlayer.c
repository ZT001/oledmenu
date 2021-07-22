/*********************************************************************************************
 *
 *绘制主界面，根据界面需求自行设计
 *2019.05.06
 *zwt
 *
**********************************************************************************************/
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "fsl_rtc.h"

#include "rc_key.h"
#include "rc_ssd1306.h"
#include "rc_common.h"
#include "rc_delay.h"

#include "rc_ui_mainlayer.h"
#include "rc_uipravite.h"

#include "project.h"

/*****************************************************************
 * private param
 * **************************************************************/
static uint8_t _flashcount=0;
/*****************************************************************
 * public param
 * **************************************************************/
/*****************************************************************
 * private param
 * **************************************************************/
/*****************************************************************
 * public function
 * **************************************************************/

/*****************************************************************
 * private function
 * **************************************************************/
/*绘制主界面****************************************************************************************
	根据项目需要自行设计部分
**/
/* 绘制主界面 */
/*绘制主界面*/
void DrawMainLayer(uint8_t clear)
{
	int currentpageid=GetMainLayerCurrentPageID() ;
	char str[20]={0};
	uint8_t line=0;
  // clear
  if (clear)
	{
		/*OLED 初始化,某些oled初始化不可靠*/
		//OLED_Init();
		OLED_CLS();
	}

//  // primeval

	/* system status temp ,humdity*/
//	else if (currentpageid == pagenum-1)
  {
		/*head softver*/
		sprintf(str,"sVer: v%d.%d%d ",SOFT_VER_MAIN,SOFT_VER_SUB1,SOFT_VER_SUB2);
		OLED_DrawTextEn(0,line++,OLED_TEXT_SIZE_6_8,str,0,-1,1);
		/*signal*/
		if(_PROJECT_cfg.UP_InterfaceType==Interface_LTE_ME3630_ZHCN)
		{
			sprintf(str,"RSSI: %d %%",_PROJECT_status.Signal);
			OLED_DrawTextEn(0,line++,OLED_TEXT_SIZE_6_8,str,0,-1,0);
		}
		/*error code*/
		/*闪烁效果*/
		if(_flashcount<5)
		{
			sprintf(str,"sMSG:                ");/*清除*/
			OLED_DrawTextEn(0,line++,OLED_TEXT_SIZE_6_8,str,0,-1,0);
		}
		else
		{
			sprintf(str,"sMSG: %s",_PROJECT_status.systemMSG);
			OLED_DrawTextEn(0,line++,OLED_TEXT_SIZE_6_8,str,0,-1,0);
		}
		_flashcount++;
		if(_flashcount>20)
			_flashcount=0;
		
//		/*module softver*/
//		if(_RCD01_cfg.UP_ComType==INTERFACE_TYPE_LTE ||
//			_RCD01_cfg.UP_ComType==INTERFACE_TYPE_LORA ||
//		_RCD01_cfg.UP_ComType==INTERFACE_TYPE_WIFI
//			)
//		{
//			sprintf(str,"IMEI%s",_RCD01_status.Up_IMEI);
//			OLED_DrawTextEn(0,5,OLED_TEXT_SIZE_6_8,str,0,-1,0);
//			sprintf(str,"%s",_RCD01_status.UP_ModuleSoftVer);
//			OLED_DrawTextEn(0,6,OLED_TEXT_SIZE_6_8,str,0,-1,0);
//		}
		//datatime
		rtc_datetime_t dt;
		RTC_GetDatetime(RTC,&dt);
		sprintf(str,"%02d-%02d-%02d %02d:%02d:%02d",dt.year,dt.month,dt.day,dt.hour,dt.minute,dt.second);
		OLED_DrawTextEn(0,7,OLED_TEXT_SIZE_6_8,str,0,-1,0);
		
		
  }

}
