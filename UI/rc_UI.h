/*****************************************************************************************
 *
 * UI public
 * 2019.05.06
 * zwt
 *
 * ***************************************************************************************/
#ifndef _ZTUI_H_
#define _ZTUI_H_

#include <stdint.h>

#define UI_LANGUAGE_EN		0x01
#define UI_LANGUAGE_CN		0x00
#define UI_DEFAULT_LANGUAGE	UI_LANGUAGE_CN

#define UI_ITEM_DEFAULT_SIZE_EN OLED_TEXT_SIZE_8_16
#define UI_ITEM_DEFAULT_SIZE_CN OLED_TEXT_SIZE_16_16

#define UI_ITEM_ENABLE	0x01
#define UI_ITEM_DISABLE	0x00
//item类型
#define ITEM_TYPE_MAIN_LAYER				0X00	//主界面
#define ITEM_TYPE_PASS_INPUT				0x01	//密码节点控件，绘制调用NUM控件
#define ITEM_TYPE_MENU_LABLE 				0x02  //菜单节点
#define ITEM_TYPE_NUM 							0x03	//数字输入控件
#define ITEM_TYPE_CHECK							0x05	//check控件
#define ITEM_TYPE_RADIO							0x06	//radio控件
#define ITEM_TYPE_BUTTON						0x07  //按钮控件，用来执行指令
#define ITEM_TYPE_MESSAGE_BOX_ITEM	0x08  //消息框控件
#define ITEM_TYPE_DI_CFG						0x09	//开关量默认值控件

//

/*****************************************************************
 * public param
 * **************************************************************/
 /*主界面显示的实时数据，根据需求修改*/
//typedef struct UIActData
//{
//	uint8_t signal;
//	double 	ai[16];
//	uint8_t aiEnable[16];
//	uint8_t aiUnit[16];
//	uint8_t di[12];
//	uint8_t diEnable[16];
//	uint8_t diType[12];
//	uint8_t	diId[12];
//	double   meter[16];
//	uint8_t meterUnit[16];
//	double Temp;/*采集盒温度*/
//	double Humidity;/*湿度*/
//} sUIActData;

 /*UI Item 数据结构*/
typedef struct UIItemConst
{
	//由上位机编辑内容
	int16_t id;			 						//item编码
	int16_t parent;		 					//父节点，0-无，
	uint8_t itemType;						//按键类型  lable,text,check,ip...
	char  text[16];			 				//item显示或输入内容
	uint16_t cmd;		 						//按键操作指令，与处理进程共同约定，处理按键事件及数据
	uint8_t	zhCNlength;				  //中文字符长度
	uint8_t zhCNAddr[5];				//中文字符地址，一个字一个地址
} sUIItemConst;

//extern sUIActData _ui_act_data;
/*
 * UIInit:初始化UI
 * void(*getdatacallback)		数据请求回调函数，用以UI原件请求显示数据
 * uint8_t cmd,							传给回调函数的指令
 * char* outdata						回调函数传出的数据指针
 * ,
 * void(*updatadatacallback) 数据跟新回调函数，用以更新UI数据存储、命令执行等操作
 * uint8_t cmd,							传给回调函数的指令
 * char* indata							传给回调函数的数据指针
 * char* outdata						回调函数传出的数据指针
*/
void UIInit(
						void (*getdatacallback)(uint16_t cmd, char *outdata),
						void (*updatadatacallback)(uint16_t cmd, char *indata,char *outdata)
	);

/* UISetLanguage:启动UI */
void UISetLanguage(uint8_t language); 
						
/* UIStart:启动UI */
void UIStart(uint8_t isshowlogo); 

/* UIClose://关闭UI */
void UIClose(void);

/* UIMainLayerRefersh://刷新主界面显示n */
void UIMainLayerRefersh(uint8_t pagenum);
						
/*
 * OnKeyPress:					监控按键事件并处理
 *											通过按键事件触发，根据当前选择item和按键类型进行事务分发处理，输入，启动事件等
 *											例如：text类型 item1选中时，接收到按键类型为next，则切换字符，
 *											为confirm时，切换字符位置，为quit时，存储或更新text数据
*/
void UIOnKeyPress(uint8_t keystatus,uint32_t key1time,uint32_t key2time); //ui的监控过程，根据按键响应，调用drawUI或item处理函数刷新界面，

void SetUIItemEnable(uint16_t id,uint8_t enable);

#endif
