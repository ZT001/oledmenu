#ifndef _RC_UI_PRIVATE_H_
#define _RC_UI_PRIVATE_H_
/*****************************************************************************************
 *
 * UI private
 * 2019.05.06
 * zwt
 *
 * ***************************************************************************************/


#include <stdint.h>
#include "rc_ui.h"

//key type
#define KEY_CMD_TYPE_NOTHING 0x00 //无点击
#define KEY_CMD_TYPE_CLICK 0x01   //set键点击
#define KEY_CMD_TYPE_CONFIRM 0x02 //set键>600ms
#define KEY_CMD_TYPE_CIRCLE 0x03  //circle健点击
#define KEY_CMD_TYPE_RETURN 0x04  //circle长按>600,<1200
#define KEY_CMD_TYPE_QUIT 0x05    //circle长按>1200
#define KEY_CMD_TYPE_TESTMOD 0x06 //circle长按>10000
#define KEY_CMD_TYPE_DISABLE 0x07 //set & circle长按>10000
//key event
#define KEY_EVENT_TYPE_NOTHING 0x00          
#define KEY_EVENT_TYPE_SHOW_MENU 0x01  
#define KEY_EVENT_TYPE_MENU_CIRCLE 0x02      
#define KEY_EVENT_TYPE_MENU_RETURN 0x03      
#define KEY_EVENT_TYPE_MENU_QUIT 0x04        
#define KEY_EVENT_TYPE_MENU_LABEL_ENTER 0x05 
#define KEY_EVENT_TYPE_PASS_CHAR_CIRCLE 0x06 
#define KEY_EVENT_TYPE_PASS_CHAR_NEXT 0x07   
#define KEY_EVENT_TYPE_PASS_CONFIRM 0x08 
#define KEY_EVENT_TYPE_NUM_CHAR_CIRCLE 0x09  
#define KEY_EVENT_TYPE_NUM_CHAR_NEXT 0x10    
#define KEY_EVENT_TYPE_NUM_CONFIRM 0x11      
#define KEY_EVENT_TYPE_CHECKLIST_CHECK 0x12
#define KEY_EVENT_TYPE_CHECKLIST_CONFIRM 0x13 
#define KEY_EVENT_TYPE_RADIOLIST_CHECK 0x14
#define KEY_EVENT_TYPE_RADIOLIST_CONFIRM 0x15
#define KEY_EVENT_TYPE_ENTER_TEST_MODE 0x16 
#define KEY_EVENT_TYPE_DEVICE_DISABLE 0x17   
#define KEY_EVENT_TYPE_BUTTON_CLICK			0x018
#define KEY_EVENT_TYPE_MESSAGEBOX_CIRCLE 0x19
#define KEY_EVENT_TYPE_MESSAGEBOX_CONFIRM 0x20 
#define KEN_EVENT_TYPE_PAGE_HOLD 0x21
#define KEN_EVENT_TYPE_PAGE_DOWN 0x22
#define KEY_EVENT_TYPE_DICONFIG_NEXT 0x23 
#define KEY_EVENT_TYPE_DICONFIG_CHECK 0x24
#define KEY_EVENT_TYPE_DICONFIG_CONFIRM 0x25
#define MESSAGEBOX_RESULT_OK 0x26
#define MESSAGEBOX_RESULT_CANCEL 0x27

//ui预设值
#define UI_MAX_ITEMS 256
#define UI_MAX_LAYER_ITEMS 32
#define UI_MAX_COLUMN 0x02           //每页列数
#define UI_MAX_ROW 0x08              //最大行数8
#define UI_MAX_ITEM_ROW 0x04         //最菜单行数4
#define UI_ROW_HEIGHT 0x02           //行高，128*64，64默认分为8page（行），此处行高指一行占据几个page
#define UI_MAX_LAYER_SHOW_ITEMS 0x08 //(UI_MAX_ROW/UI_ROW_HEIGHT*UI_MAX_COLUMN)

#define UI_MAIN_LAYER_ID -2
#define UI_PASS_LAYER_ID -1
#define UI_MENU_LAYER_ID 0

#define UNCLEAR_OLED 0
#define CLEAR_OLED 1

//#define ITEM_UNUSE_DEFAULT_VALUE 				0
//#define ITEM_USE_DEFAULT_VALUE 					1

/*****************************************************************
 * public function
 * **************************************************************/
/*raycoUI.h*/
/*****************************************************************
 * private function
 * **************************************************************/
//key event
uint8_t GetKeyCmdType(uint8_t itemtype, uint8_t keystatus, int32_t key1time, int32_t key2time);

//界面数据
void GetChilds(int16_t itemid);
uint8_t GetChildCount(uint16_t itemid); //从原件库中检索层中指定序号元素所包含的子节点数量
void GetItemDefaultValue(uint16_t cmd,char *data);       /*获取控件默认值  在进入密码界面和返回操作时调用 */
int GetPathSelectedLevel(void);

//界面绘制
void DrawMainLayer(uint8_t clear);
uint8_t GetMainLayerCurrentPageID(void);
uint8_t GetMainLayerPageNumber(void);
void OnMainLayerPageHold(void);
void OnMainLayerPageDown(void);
//void UIMainLayerRefersh(sUIActData data); public
void OnKeyEnterMenu(void);
void OnKeyTestMode(void); //内部测试

//密码输入
void DrawPassLayer(uint8_t isclear);
void OnPassCircleChar(void);                                                        //key event
void OnPassNextChar(void);                                                          //key event
void OnKeyConfirmPass(void (*compFunct)(uint16_t cmd, char *indata, char *outdata)); //key event
//菜单
void DrawMenu(void);                                      //绘制节点
void DrawMenuItem(uint16_t itemindex, int x, int row, uint8_t reverse); //item 调用元件
void OnMenuItemCircle(void);                                       //key event
void OnMenuItemRerurn(void);                                       //key event
void OnMenuQuit(void);                                             //key event

//control
//menu lable
void DrawMenuLabel(uint16_t itemindex, int x, int row, uint8_t reverse);
void OnMenuLabelEnter(void);

//num input
void DrawNumInput(uint16_t itemindex, int x, int row, uint8_t isclear);
void OnNumCircleChar(void); //key event
void OnNumNextChar(void);   //key event
void OnNumConfirm(void (*compFunct)(uint16_t cmd, char *indata, char *outdata));

//check list
void DrawCheck(uint16_t itemindex, int x, int row, uint8_t reverse, uint8_t isclear);
void DrawCheckHead(int x, int row, char enable);
void OnCheckCheck(void (*compFunct)(uint16_t cmd, char *indata, char *outdata)); //key event

//radio list
void DrawRadio(uint16_t itemindex, int x, int row, uint8_t reverse, uint8_t isclear);
void DrawRadioHead(int x, int row, char enable);
void OnRadioCheck(void (*compFunct)(uint16_t cmd, char *indata, char *outdata)); //key event

//di config
void DrawDIConfig(uint16_t itemindex, int x, int row,uint8_t isclear);
void DrawDIConfigHead(int x, int row, char enable);
void OnDIConfigNextBit(void);   //key event
void OnDIConfigChangeBit(void (*compFunct)(uint16_t cmd, char *indata, char *outdata));   //key event
void OnDIConfigConfirm(void (*compFunct)(uint16_t cmd, char *indata, char *outdata)); //key event

//button
void DrawButton(uint16_t itemindex, int x, int row, uint8_t reverse);
void OnButtonClick(void (*compFunct)(uint16_t cmd, char *indata, char *outdata)); //key event
//
int8_t DrawMessage(uint16_t itemindex);/*item为调用的button*/
//signal icon
void DrawSignal(uint8_t power);
//power icon
void DrawPower(uint8_t power);
//ligth icon
void DrawDILight(int x, int row, char enable);
/*logo*/
void DrawLogo(void);

#endif
