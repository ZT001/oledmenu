/*********************************************************************************************
 *
 *构建菜单，用以绘制菜单、处理菜单按键事件及显示。包含按键处理、OLED显示、菜单绘制三部分
 *2019.05.06
 *zwt
 *
**********************************************************************************************/
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "rc_key.h"
#include "rc_ssd1306.h"
#include "rc_common.h"
#include "rc_delay.h"

#include "rc_menudata.h"
#include "rc_UIpravite.h"

/*****************************************************************
 * private param
 * **************************************************************/
#define SELECTED_CHILD_ID               \
  _menu_path_item[_path_selected_level] \
      .childsId[_menu_path_item[_path_selected_level].selectedIndex]
			
#define MAX_LAYER	7

/*当前层级节点数据状态*/
typedef struct UIItemChilds
{
  uint16_t itemId;
  int16_t parent;
  uint16_t childsId
      [UI_MAX_LAYER_ITEMS]; //当前页面的元素,在页面层级发生变化时重新填充//用以减少基础数据查询次数
  uint16_t childCount;       // childs中元素数量
  uint8_t selectedIndex;    // childs中当前选中元素index
} sUIItemChilds;

/*密码元件*/

/*界面元素*/
static uint16_t _ui_item_count = 0; //
//static sUIItem *_ui_items = 0;      //所有界面元素，在启动时，从flash加载 由prfile填充
/*实时数据数组，用以刷新主界面显示*/
//sUIActData _ui_act_data = {0};
static int _mainLayerPageNumber;
static int _mainLayerSelectedPageID;
static uint32_t ccstart = 0;/*页面切换计时*/
static uint32_t ccnow = 0;
#define MAIN_LAYER_PAGE_HOLDING_TIME	60000/*主界面set键page保持时间*/
#define MAIN_LAYER_PAGE_INTERVAL	5000			/*主界面自动切换page间隔*/
static uint32_t _mainLayerPageShiftInterval=MAIN_LAYER_PAGE_INTERVAL;
static uint8_t _mainLayerNeedClear=UNCLEAR_OLED;

/*菜单操作路径*/
static sUIItemChilds _menu_path_item[MAX_LAYER] = {0}; //当前选中元素
static int8_t _path_selected_level = UI_MAIN_LAYER_ID; //用来区分位置0-menu,-1-password,-2-main
/*选中元素数据交换缓存*/
static uint8_t _selectedItem_selected_charIndex=0;
static char _selectedItem_data[20]={0};
/*菜单数据处理回调函数接口*/
static void (*_UIGetData)(uint16_t cmd, char *outdata);
static void (*_UIUpdataData)(uint16_t cmd,char *indata, char *outdata);
/*菜单超时 30s*/
static uint32_t _menuTimeOut=60;//s
static uint32_t _menuLastClickTime=0;//s
/*logo*/
static uint8_t _language=0;/*0-cn,1-en*/
/*message box*/
static const sUIItemConst _messageItems[2]={{0,0,ITEM_TYPE_MESSAGE_BOX_ITEM,"OK",0,2,93,94,0,0,0},/*ok*/
																{1,0, ITEM_TYPE_MESSAGE_BOX_ITEM,"Cancel",1,2,95,96,0,0,0}};/*cancel*/
static uint8_t _selectedMessageItemid=1;/*0-ok,1-cancel，使用完一次后赋值为0*/
static int8_t _messageBoxResult=-1;/*-1-未选择,0-ok,1-cancel,使用完一次后赋值为-1以清除状态*/
static SemaphoreHandle_t _UIRefreshSemaphore;//按键动作与界面绘制
/**/
/*****************************************************************
 * public param
 * ************************************************************ **/
/**/

/*****************************************************************
 * public function
 * ************************************************************ **/
/*初始化*/
void UIInit(void (*getdatacallback)(uint16_t cmd, char *outdata),
						void (*updatadatacallback)(uint16_t cmd, char *indata,char *outdata)
						)
{
	_UIRefreshSemaphore=xSemaphoreCreateMutex();
	/*OLED 初始化*/
  OLED_Init();
	/*key gpio初始化*/
  //KeyInit();
	/*获取菜单元素长度*/
	_ui_item_count = sizeof(_ui_items_c)/sizeof(sUIItemConst);
	/*初始化_ui_item_p*/
	for(int i=0;i<_ui_item_count;i++)
	{
		_ui_items_enable[i]=1;
	}
	/*菜单数据处理函数关联*/
  _UIGetData=getdatacallback;
	_UIUpdataData=updatadatacallback;
}
/*语言切换，0-cn，1-en*/
void UISetLanguage(uint8_t language)
{
	_language=language;
}
/*启动UI*/
void UIStart(uint8_t isshowlogo)
{
  /*绘制logo*/
	if(isshowlogo)
	{
		DrawLogo();
		OLED_CLS();
	}
	/*进入主界面*/
  _path_selected_level = UI_MAIN_LAYER_ID;
  DrawMainLayer(CLEAR_OLED);
}
/*关闭UI*/
void UIClose(void)
{
  OLED_CLS();
  OLED_OFF();
}
/* 主界面绘制*/
void UIMainLayerRefersh(uint8_t pagenum)
{
	/*等待信号量释放，防止和OnKeyPress任务冲突*/
	xSemaphoreTake(_UIRefreshSemaphore,portMAX_DELAY);
	
  if (GetPathSelectedLevel() == UI_MAIN_LAYER_ID) //主界面才刷新，配置界面跳过
  {
		/*赋值*/
		_mainLayerPageNumber=pagenum;
		
    ccnow = xTaskGetTickCount();
    if ((ccnow - ccstart) * portTICK_PERIOD_MS > _mainLayerPageShiftInterval)
    {
      _mainLayerSelectedPageID ++;
			if(_mainLayerSelectedPageID>=pagenum)
			{
				_mainLayerSelectedPageID=0;
			}
			
      ccstart = ccnow;
			
			if(pagenum>1)
				_mainLayerNeedClear=CLEAR_OLED; 
			/*页面保持时间到后，更换回自动切换间隔*/
			if(_mainLayerPageShiftInterval==MAIN_LAYER_PAGE_HOLDING_TIME)
				_mainLayerPageShiftInterval=MAIN_LAYER_PAGE_INTERVAL;
    }
		
		DrawMainLayer(_mainLayerNeedClear);
		_mainLayerNeedClear=UNCLEAR_OLED;
  }
	/*长时间无按键则退回mainlayer*/
	else if(_path_selected_level>UI_MAIN_LAYER_ID)
	{
			if((xTaskGetTickCount()-_menuLastClickTime)>pdMS_TO_TICKS(_menuTimeOut*1000))
			{
				ccstart=xTaskGetTickCount();
				_mainLayerSelectedPageID=0;
				_path_selected_level=UI_MAIN_LAYER_ID;
				memset(&_menu_path_item, 0,sizeof(sUIItemChilds)*MAX_LAYER); //清空当前节点
				OLED_CLS();
			}
	}
	/*释放信号量*/
	xSemaphoreGive(_UIRefreshSemaphore);
}

/*按键事件处理，需循环监视*/
void UIOnKeyPress(uint8_t keystatus,uint32_t key1time,uint32_t key2time)
{
  uint8_t keytype = KEY_EVENT_TYPE_NOTHING;
	/*等待信号量释放，方式和UIMainLayerRefer任务冲突*/
	xSemaphoreTake(_UIRefreshSemaphore,portMAX_DELAY);
	/*有按键动作则刷新最后按键时间*/
	_menuLastClickTime=xTaskGetTickCount(); 
  //否则，获取按键操作类型
	if(_path_selected_level<0)/*密码界面和主界面无元素类型*/
		keytype = GetKeyCmdType(NULL,keystatus, key1time, key2time);
	else
		keytype = GetKeyCmdType(_ui_items_c[SELECTED_CHILD_ID].itemType,keystatus, key1time, key2time);

  switch (keytype)
  {
  case KEY_EVENT_TYPE_NOTHING:
    break;
	case KEN_EVENT_TYPE_PAGE_HOLD:
		OnMainLayerPageHold();
		break;
	case KEN_EVENT_TYPE_PAGE_DOWN:
		OnMainLayerPageDown();
		break;
  case KEY_EVENT_TYPE_SHOW_MENU:
    OnKeyEnterMenu();
    break;
  case KEY_EVENT_TYPE_PASS_CHAR_CIRCLE:
    OnPassCircleChar();
    break;
  case KEY_EVENT_TYPE_PASS_CHAR_NEXT:
    OnPassNextChar();
    break;
  case KEY_EVENT_TYPE_PASS_CONFIRM:
    OnKeyConfirmPass(_UIUpdataData);
    break;
  case KEY_EVENT_TYPE_MENU_CIRCLE:
    OnMenuItemCircle();
    break;
  case KEY_EVENT_TYPE_MENU_RETURN:
    OnMenuItemRerurn();
    break;
  case KEY_EVENT_TYPE_MENU_QUIT:
    OnMenuQuit();
    break;
  case KEY_EVENT_TYPE_MENU_LABEL_ENTER:
    OnMenuLabelEnter();
    break;
  case KEY_EVENT_TYPE_NUM_CHAR_CIRCLE:
    OnNumCircleChar();
    break;
  case KEY_EVENT_TYPE_NUM_CHAR_NEXT:
    OnNumNextChar();
    break;
  case KEY_EVENT_TYPE_NUM_CONFIRM:
    OnNumConfirm(_UIUpdataData);
    break;
  case KEY_EVENT_TYPE_CHECKLIST_CHECK:
    OnCheckCheck(_UIUpdataData);
    break;
	case KEY_EVENT_TYPE_CHECKLIST_CONFIRM:
    //OnCheckCheck(_UIUpdataData);
		OnMenuItemRerurn();
    break;
  case KEY_EVENT_TYPE_RADIOLIST_CHECK:
    OnRadioCheck(_UIUpdataData);
    break;
	case KEY_EVENT_TYPE_RADIOLIST_CONFIRM:
    //OnRadioCheck(_UIUpdataData);
		OnMenuItemRerurn();
    break;
	case KEY_EVENT_TYPE_DICONFIG_CHECK:
		OnDIConfigChangeBit(_UIUpdataData);
		break;
	case KEY_EVENT_TYPE_DICONFIG_NEXT:
		OnDIConfigNextBit();
		break;
	case KEY_EVENT_TYPE_DICONFIG_CONFIRM:
		OnDIConfigConfirm(_UIUpdataData);
		OnMenuItemRerurn();
		break;
	case KEY_EVENT_TYPE_BUTTON_CLICK:
			OnButtonClick(_UIUpdataData);
  case KEY_EVENT_TYPE_ENTER_TEST_MODE:
    OnKeyTestMode();
    break;
  }
	/*释放信号量*/
	xSemaphoreGive(_UIRefreshSemaphore);
}
/*****************************************************************
 * private function
 * **************************************************************/
/*按键事件类型解析*/
uint8_t GetKeyCmdType(uint8_t itemtype, uint8_t keystatus,
                      int32_t key1time, int32_t key2time)
{
  int keytype = 0;
  /* key type */
  if (keystatus == KEY_STATUS_SET)
  { //set
    if (key1time < 400)
      keytype = KEY_CMD_TYPE_CLICK;
    else if (key1time >= 400 && key1time < 5000)
      keytype = KEY_CMD_TYPE_CONFIRM;
  }
  else if (keystatus == KEY_STATUS_CIRCLE)
  { //circle
    if (key2time < 400)
      keytype = KEY_CMD_TYPE_CIRCLE;
    else if (key2time >= 400 && key2time < 2000)
      keytype = KEY_CMD_TYPE_RETURN;
    else if (key2time > 2000 && key2time < 5000)
      keytype = KEY_CMD_TYPE_QUIT;
    else if (key2time > 5000)
      keytype = KEY_CMD_TYPE_TESTMOD;
  }
  else if (keystatus == KEY_STATUS_ALL)
  { //set & circle
    if (key1time > 10000 && key2time > 10000)
      keytype = KEY_CMD_TYPE_DISABLE;
  }
	
  /* key event type */
	/*主界面按键动作*/
  if (_path_selected_level == UI_MAIN_LAYER_ID) 
  {
    if (keytype == KEY_CMD_TYPE_CLICK)
      return KEN_EVENT_TYPE_PAGE_HOLD;
		if (keytype == KEY_CMD_TYPE_CIRCLE)
      return KEN_EVENT_TYPE_PAGE_DOWN;
    else if (keytype == KEY_CMD_TYPE_CONFIRM)
      return KEY_EVENT_TYPE_SHOW_MENU;
    else if (keytype == KEY_CMD_TYPE_TESTMOD)
      return KEY_EVENT_TYPE_ENTER_TEST_MODE;
    else if (keytype == KEY_CMD_TYPE_DISABLE)
      return KEY_EVENT_TYPE_DEVICE_DISABLE;
		else
			return KEY_EVENT_TYPE_NOTHING;
  }/*pass*/
	if(_path_selected_level == UI_PASS_LAYER_ID)
	{
		if (keytype == KEY_CMD_TYPE_CLICK)
			return KEY_EVENT_TYPE_PASS_CHAR_NEXT;
		else if (keytype == KEY_CMD_TYPE_CIRCLE)
			return KEY_EVENT_TYPE_PASS_CHAR_CIRCLE;
		else if (keytype == KEY_CMD_TYPE_CONFIRM)
			return KEY_EVENT_TYPE_PASS_CONFIRM;
		else if (keytype == KEY_CMD_TYPE_RETURN)
			return KEY_EVENT_TYPE_MENU_QUIT;
		else if (keytype == KEY_CMD_TYPE_QUIT)
			return KEY_EVENT_TYPE_MENU_QUIT;
		else
			return KEY_EVENT_TYPE_NOTHING;
	}
	/*菜单界面按键动作*/
  else 
  {
    switch (itemtype)
    {
    case ITEM_TYPE_MENU_LABLE:/*menu label*/
      if (keytype == KEY_CMD_TYPE_CLICK)
        return KEY_EVENT_TYPE_MENU_LABEL_ENTER;
      else if (keytype == KEY_CMD_TYPE_CIRCLE)
        return KEY_EVENT_TYPE_MENU_CIRCLE;
			else if (keytype == KEY_CMD_TYPE_RETURN)
				return KEY_EVENT_TYPE_MENU_RETURN;
			else if (keytype == KEY_CMD_TYPE_QUIT)
				return KEY_EVENT_TYPE_MENU_QUIT;
      break;
		case ITEM_TYPE_PASS_INPUT: /*pass*/
			if (keytype == KEY_CMD_TYPE_CLICK)
				return KEY_EVENT_TYPE_PASS_CHAR_NEXT;
			else if (keytype == KEY_CMD_TYPE_CIRCLE)
				return KEY_EVENT_TYPE_PASS_CHAR_CIRCLE;
			else if (keytype == KEY_CMD_TYPE_CONFIRM)
				return KEY_EVENT_TYPE_PASS_CONFIRM;
			else if (keytype == KEY_CMD_TYPE_RETURN)
				return KEY_EVENT_TYPE_MENU_QUIT;
			else if (keytype == KEY_CMD_TYPE_QUIT)
				return KEY_EVENT_TYPE_MENU_QUIT;
			break;
    case ITEM_TYPE_NUM: /*num*/
      if (keytype == KEY_CMD_TYPE_CLICK)
        return KEY_EVENT_TYPE_NUM_CHAR_NEXT;
      else if (keytype == KEY_CMD_TYPE_CIRCLE)
        return KEY_EVENT_TYPE_NUM_CHAR_CIRCLE;
      else if (keytype == KEY_CMD_TYPE_CONFIRM)
        return KEY_EVENT_TYPE_NUM_CONFIRM;
      break;
    case ITEM_TYPE_CHECK: /*check list*/
      if (keytype == KEY_CMD_TYPE_CLICK)
        return KEY_EVENT_TYPE_CHECKLIST_CHECK;
      else if (keytype == KEY_CMD_TYPE_CIRCLE)
        return KEY_EVENT_TYPE_MENU_CIRCLE;
			else if (keytype == KEY_CMD_TYPE_CONFIRM)
        return KEY_EVENT_TYPE_CHECKLIST_CONFIRM;
      break;
    case ITEM_TYPE_RADIO: /*radio list*/
      if (keytype == KEY_CMD_TYPE_CLICK)
        return KEY_EVENT_TYPE_RADIOLIST_CHECK;
      else if (keytype == KEY_CMD_TYPE_CIRCLE)
        return KEY_EVENT_TYPE_MENU_CIRCLE;
			else if (keytype == KEY_CMD_TYPE_CONFIRM)
        return KEY_EVENT_TYPE_RADIOLIST_CONFIRM;
      break;
		case ITEM_TYPE_DI_CFG: /*di config*/
      if (keytype == KEY_CMD_TYPE_CLICK)
        return KEY_EVENT_TYPE_DICONFIG_CHECK;
      else if (keytype == KEY_CMD_TYPE_CIRCLE)
        return KEY_EVENT_TYPE_DICONFIG_NEXT;
			else if (keytype == KEY_CMD_TYPE_CONFIRM)
        return KEY_EVENT_TYPE_DICONFIG_CONFIRM;
      break;
		case ITEM_TYPE_BUTTON:/*button相应的事件*/
			if (keytype == KEY_CMD_TYPE_CLICK)
        return KEY_EVENT_TYPE_BUTTON_CLICK;
			else if (keytype == KEY_CMD_TYPE_CIRCLE)
        return KEY_EVENT_TYPE_MENU_CIRCLE;
			else if (keytype == KEY_CMD_TYPE_RETURN)
				return KEY_EVENT_TYPE_MENU_QUIT;
			else if (keytype == KEY_CMD_TYPE_QUIT)
				return KEY_EVENT_TYPE_MENU_QUIT;
		case ITEM_TYPE_MESSAGE_BOX_ITEM:/*messagebox相应的事件*/
			if (keytype == KEY_CMD_TYPE_CLICK)
				return KEY_EVENT_TYPE_MESSAGEBOX_CONFIRM;
			else if (keytype == KEY_CMD_TYPE_CIRCLE)
				return KEY_EVENT_TYPE_MESSAGEBOX_CIRCLE;
			break;
    }
  }
  return KEY_CMD_TYPE_NOTHING;
}

void SetUIItemEnable(uint16_t id,uint8_t enable)
{
	if(id<0 || id>_ui_item_count)
		return;
	_ui_items_enable[id]=enable;
}
/* 获得子节点 */
void GetChilds(int16_t itemid)
{
  int i = 0, n = 0;
  for (i = 0; i < _ui_item_count; i++)
  {
    if (_ui_items_c[i].parent == itemid && _ui_items_enable[i]==UI_ITEM_ENABLE)
    {
      _menu_path_item[_path_selected_level].childsId[n] = _ui_items_c[i].id;
      n++;
    }
  }
  _menu_path_item[_path_selected_level].childCount = n;
	
	/*clear data*/
	_selectedItem_selected_charIndex=0;
	memset(_selectedItem_data,0,16);
}

/*获取child节点数量*/
uint8_t GetChildCount(uint16_t itemid)
{
  int i = 0, n = 0;
  for (i = 0; i < _ui_item_count; i++)
  {
    if (_ui_items_c[i].parent == itemid && _ui_items_enable[i]==UI_ITEM_ENABLE)
      n++;
  }
  return n;
}

int GetPathSelectedLevel()
{
	return _path_selected_level;
}
/*Main layer******************************************************************************************/
uint8_t GetMainLayerCurrentPageID(void)
{
	return _mainLayerSelectedPageID;
}
uint8_t GetMainLayerPageNumber(void)
{
	return _mainLayerPageNumber;
}

/*菜单******************************************************************************************/
/*绘制菜单*/
void DrawMenu(void)
{
  uint8_t reserve = 0;
  uint8_t n = 0;
	int i = 0;
	uint8_t clear = UNCLEAR_OLED;
	uint8_t layermaxitemcount=UI_MAX_LAYER_SHOW_ITEMS;

	/*检查元素显示长度，确定每页显示元素数量*/
	for(int i=0;i<_menu_path_item[_path_selected_level].childCount;i++)
	{
		if(_language==0)
		{
			if(_ui_items_c[_menu_path_item[_path_selected_level].childsId[i]].zhCNlength>=4)
			{
				layermaxitemcount=4;/*4行1列*/
				break;
			}
		}
		else
		{
			if(strlen(_ui_items_c[_menu_path_item[_path_selected_level].childsId[i]].text)>=8)
			{
				layermaxitemcount=4;/*4行1列*/
				break;
			}
		}
	}
  // clear
  if (_menu_path_item[_path_selected_level].selectedIndex % layermaxitemcount == 0 &&
		_menu_path_item[_path_selected_level].selectedIndex!=0)
    clear = CLEAR_OLED;
  // to first clear
  if (_menu_path_item[_path_selected_level].selectedIndex >
      _menu_path_item[_path_selected_level].childCount - 1)
  {
    _menu_path_item[_path_selected_level].selectedIndex = 0;
    clear = CLEAR_OLED;
  }
  

  if (_menu_path_item[_path_selected_level].childCount > 0)
  {
    //清除
    if (clear)
      OLED_CLS();
    /*计算当前页显示元素数量*/
    n = _menu_path_item[_path_selected_level].childCount -
                    (_menu_path_item[_path_selected_level].selectedIndex /
                     layermaxitemcount * layermaxitemcount) >
                layermaxitemcount
            ? layermaxitemcount
            : (_menu_path_item[_path_selected_level].childCount -
               _menu_path_item[_path_selected_level].selectedIndex /
                   layermaxitemcount * layermaxitemcount);
    for (i = 0; i < n; i++)
    {
			/*计算当前元素序号*/
      reserve =
          (_menu_path_item[_path_selected_level].selectedIndex ==
                   (i + (_menu_path_item[_path_selected_level].selectedIndex /
                         layermaxitemcount) *
                            layermaxitemcount)
               ? 1
               : 0);
			/*绘制元素*/
      DrawMenuItem(_menu_path_item[_path_selected_level].childsId
                  [i + (_menu_path_item[_path_selected_level].selectedIndex /
                        layermaxitemcount) *
                           layermaxitemcount],
          i / 4 * 64, (i % UI_MAX_ITEM_ROW) * 2, reserve);
    }
  }
}
/*绘制菜单元素*/
void DrawMenuItem(uint16_t itemindex, int x, int row, uint8_t reverse)
{
  switch (_ui_items_c[itemindex].itemType)
  {
  case ITEM_TYPE_MENU_LABLE:
    DrawMenuLabel(itemindex, x, row, reverse);
    break;
  case ITEM_TYPE_NUM:
    DrawNumInput(itemindex, x, row, UNCLEAR_OLED);
    break;
  case ITEM_TYPE_CHECK:
    DrawCheck(itemindex, x, row, reverse, CLEAR_OLED);
    break;
  case ITEM_TYPE_RADIO:
    DrawRadio(itemindex, x, row, reverse, CLEAR_OLED);
    break;
	case ITEM_TYPE_DI_CFG:
    DrawDIConfig(itemindex, x, row, CLEAR_OLED);
    break;
	case ITEM_TYPE_BUTTON:
		DrawButton(itemindex,x,row,reverse);
  }
}

/*KEY_EVENT_TYPE_PAGE_Hold*/
void OnMainLayerPageHold(void)
{
	if(_mainLayerPageShiftInterval==MAIN_LAYER_PAGE_HOLDING_TIME)
	{
		_mainLayerSelectedPageID++;
		if(_mainLayerSelectedPageID>=_mainLayerPageNumber)
		{
				_mainLayerSelectedPageID=0;
		}
	}
	_mainLayerPageShiftInterval=MAIN_LAYER_PAGE_HOLDING_TIME;
	ccstart = xTaskGetTickCount();
	_mainLayerNeedClear=CLEAR_OLED;
}

/*KEY_EVENT_TYPE_PAGE_DOWN*/
void OnMainLayerPageDown(void)
{
	_mainLayerSelectedPageID++;
	if(_mainLayerSelectedPageID>=_mainLayerPageNumber)
	{
		_mainLayerSelectedPageID=0;
	}
	ccstart = xTaskGetTickCount();
	/*切换按键后，将page切换间隔调整回默认间隔*/
	_mainLayerPageShiftInterval=MAIN_LAYER_PAGE_INTERVAL;
	_mainLayerNeedClear=CLEAR_OLED;
}
/*KEY_CMD_TYPE_NEXT*/
void OnMenuItemCircle(void)
{
	_menu_path_item[_path_selected_level].selectedIndex++;
  DrawMenu();//clear);
}
/*KEY_CMD_TYPE_RETURN*/
void OnMenuItemRerurn(void)
{
	// clear
	memset(&_menu_path_item[_path_selected_level], 0,
	         sizeof(sUIItemChilds)); //清空当前节点
	
	/*返回时跳过pass界面*/
	if (_path_selected_level==UI_MENU_LAYER_ID)//
	{
	  _path_selected_level=UI_MAIN_LAYER_ID;
	}
	/*上层节点非密码控件，则退回一层*/
	else
		_path_selected_level--;

	/*如果退回到主界面，绘制主界面*/
  if (_path_selected_level == UI_MAIN_LAYER_ID)
	{
		ccstart=xTaskGetTickCount();
		_mainLayerSelectedPageID=0;
    _mainLayerNeedClear=CLEAR_OLED;
	}
  else/*否则，绘制菜单*/
  {
		if(_path_selected_level==UI_MENU_LAYER_ID)
		{
			if(_ui_items_c[_menu_path_item[_path_selected_level].childsId[0]].parent==-1)
				GetChilds(-1);/*普通密码根节点id=-1*/
			if(_ui_items_c[_menu_path_item[_path_selected_level].childsId[0]].parent==-2)
				GetChilds(-2);/*超级密码根节点id=-2*/
		}
		else
		{
			/*重新获取子节点*/
			GetChilds(
					_ui_items_c[_menu_path_item[_path_selected_level-1]
												.childsId[_menu_path_item[_path_selected_level-1]
																			.selectedIndex]]
							.id);
		}
		/*clear*/
		OLED_CLS();
    DrawMenu();//CLEAR_OLED);
  }
}
/*KEY_CMD_TYPE_QUIT*/
void OnMenuQuit(void)
{
  _path_selected_level = UI_MAIN_LAYER_ID; //不管在那个layer，一律回layer -1
  memset(&_menu_path_item, 0,
         sizeof(sUIItemChilds)*MAX_LAYER); //清空当前节点
	ccstart=xTaskGetTickCount();
	_mainLayerSelectedPageID=0;
	OLED_CLS();
}

/*  menu
 * label控件******************************************************************************************/
/*draw label*/
void DrawMenuLabel(uint16_t itemindex, int x, int row, uint8_t invert)
{
	int i=0;
	if(_language==UI_LANGUAGE_EN ||_ui_items_c[itemindex].zhCNlength==0)
		OLED_DrawTextEn(x,row,UI_ITEM_DEFAULT_SIZE_EN,(char*)_ui_items_c[itemindex].text,invert,-1,0);
	else if(_language==UI_LANGUAGE_CN)
	{
		for(i=0;i<_ui_items_c[itemindex].zhCNlength;i++)
			OLED_DrawTextCn(x+i*16,row,UI_ITEM_DEFAULT_SIZE_CN,_ui_items_c[itemindex].zhCNAddr[i],invert,0);
	}
}
/*KEY_CMD_TYPE_ENTER*/
void OnMenuLabelEnter(void)
{
  int count = 0;

  count = GetChildCount(
      _menu_path_item[_path_selected_level]
          .childsId[_menu_path_item[_path_selected_level].selectedIndex]);

  if (count > 0) //有子节点则进入下一层
  {
		if(_path_selected_level<MAX_LAYER-1)
    _path_selected_level++;

    GetChilds(
        _ui_items_c[_menu_path_item[_path_selected_level - 1]
                      .childsId[_menu_path_item[_path_selected_level - 1]
                                    .selectedIndex]]
            .id);
	
		/*clear*/
		OLED_CLS();
    DrawMenu();//CLEAR_OLED);
  }
}
/* password
 * layer******************************************************************************************/
/*KEY_CMD_TYPE_PASS*/
void OnKeyEnterMenu(void)
{
  _path_selected_level = UI_PASS_LAYER_ID;
	/*clear data*/
	_selectedItem_selected_charIndex=0;
	memset(_selectedItem_data,0,16);
	
	DrawPassLayer(pdTRUE);
}
/*KEY_CMD_TYPE_TEST_MODE*/
void OnKeyTestMode(void) 
{
	
}

/* draw password*/
void DrawPassLayer(uint8_t isclear)
{
	//char data[16];
	int i = 0, textl = 0, datal = 0;
  // clear
  if (isclear)
    OLED_CLS();
	if(_selectedItem_data[0]==0)
	{
		_UIGetData(0xFFFF,_selectedItem_data);
	}
  //get lenth  en
  for (i = 0;; i++)
  {
    if (_ui_pass_c.text[i] == '\0')
      break;
  }
  textl = i;
  for (i = 0;; i++)
  {
    if (_selectedItem_data[i] == '\0')
      break;
  }
  datal = i;
	//get lenth cn
	if(_language==UI_LANGUAGE_CN)
	{
		textl=_ui_pass_c.zhCNlength==0?textl:_ui_pass_c.zhCNlength;
	}
	
  //title
	if(_language==UI_LANGUAGE_EN ||_ui_pass_c.zhCNlength==0)
		OLED_DrawTextEn((OLED_WDITH - textl * 8) / 2, 0, UI_ITEM_DEFAULT_SIZE_EN,(char*)_ui_pass_c.text, 0, -1, 1);
	else if(_language==UI_LANGUAGE_CN)
	{
		for(i=0;i<_ui_pass_c.zhCNlength;i++)
			OLED_DrawTextCn((OLED_WDITH - textl * 16) / 2+i*16,0,UI_ITEM_DEFAULT_SIZE_CN,_ui_pass_c.zhCNAddr[i],0,1);
	}
	//data
  OLED_DrawTextEn((OLED_WDITH - datal * 8) / 2, 4, UI_ITEM_DEFAULT_SIZE_EN,_selectedItem_data, 0,_selectedItem_selected_charIndex, 0);
}
/*KEY_CMD_TYPE_NEXT_CHAR*/
void OnPassCircleChar(void)
{
	_selectedItem_data[_selectedItem_selected_charIndex]++;

	if (_selectedItem_data[_selectedItem_selected_charIndex] > '9' ||
		_selectedItem_data[_selectedItem_selected_charIndex] < '0' )
		_selectedItem_data[_selectedItem_selected_charIndex] = '0';

  DrawPassLayer(pdFALSE);
}
/*KEY_CMD_TYPE_CONFIRM_CHAR*/
void OnPassNextChar(void)
{	
	uint8_t length = 0;
  int i = 0;
  // get length
  for (i = 0;; i++)
  {
    if (_selectedItem_data[i] == '\0')
      break;
  }
  length = i;

 _selectedItem_selected_charIndex++;
  //跳过 小数点
  if (_selectedItem_data[_selectedItem_selected_charIndex] == '.')
   _selectedItem_selected_charIndex++;

  if (_selectedItem_selected_charIndex >= length)
   _selectedItem_selected_charIndex = 0;
  DrawPassLayer(pdFALSE);
}
/*KEY_CMD_TYPE_CONFIRM_PASS*/
void OnKeyConfirmPass(void (*compFunct)(uint16_t cmd, char *indata, char *outdata))
{
	char outdata=0;
  compFunct(
      _ui_pass_c.cmd,
      _selectedItem_data, &outdata);

  if (outdata == '1')/*用户密码*/
  {
		if(_path_selected_level<MAX_LAYER-1)
    _path_selected_level++; //
    GetChilds(-1);/*-1为用户菜单根节点*/
		/*clear*/
		OLED_CLS();
    DrawMenu();//CLEAR_OLED);
  }
	else if (outdata == '2')/*超级密码*/
  {
		if(_path_selected_level<MAX_LAYER-1)
    _path_selected_level++; //
    GetChilds(-2);/*-2为工厂参数菜单根节点*/
		/*clear*/
		OLED_CLS();
    DrawMenu();//CLEAR_OLED);
  }
	
}

/* number input
 * 控件******************************************************************************************/
/* draw number input*/
void DrawNumInput(uint16_t itemindex, int x, int row, uint8_t isclear)
{
  int i = 0, textl = 0, datal = 0;

	//
	if(_selectedItem_data[0]==0)
	{
		_UIGetData(_ui_items_c[itemindex].cmd,_selectedItem_data);
	}
		
  // clear
  if (isclear)
    OLED_CLS();
  //get lenth  en
  for (i = 0;; i++)
  {
    if (_ui_items_c[itemindex].text[i] == '\0')
      break;
  }
  textl = i;
  for (i = 0;; i++)
  {
    if (_selectedItem_data[i] == '\0')
      break;
  }
  datal = i;
	//get lenth cn
	if(_language==UI_LANGUAGE_CN)
	{
		textl=_ui_items_c[itemindex].zhCNlength==0?textl:_ui_items_c[itemindex].zhCNlength;
	}
	
  //title
	if(_language==UI_LANGUAGE_EN ||_ui_items_c[itemindex].zhCNlength==0)
		OLED_DrawTextEn((OLED_WDITH - textl * 8) / 2, 0, UI_ITEM_DEFAULT_SIZE_EN, (char*)_ui_items_c[itemindex].text, 0, -1, 1);
	else if(_language==UI_LANGUAGE_CN)
	{
		for(i=0;i<_ui_items_c[itemindex].zhCNlength;i++)
			OLED_DrawTextCn((OLED_WDITH - textl * 16) / 2+i*16,row,UI_ITEM_DEFAULT_SIZE_CN,_ui_items_c[itemindex].zhCNAddr[i],0,1);
	}
	//data
  OLED_DrawTextEn((OLED_WDITH - datal * 8) / 2, 4, UI_ITEM_DEFAULT_SIZE_EN, _selectedItem_data, 0, _selectedItem_selected_charIndex, 0);
}
/*KEY_CMD_TYPE_NEXT_CHAR*/
void OnNumCircleChar(void)
{
	if (_selectedItem_data[_selectedItem_selected_charIndex] =='+')
	{
    _selectedItem_data[_selectedItem_selected_charIndex] = '-';
	}
	else if (_selectedItem_data[_selectedItem_selected_charIndex] =='-')
	{
    _selectedItem_data[_selectedItem_selected_charIndex] = '+';
	}
	else
	{
		_selectedItem_data[_selectedItem_selected_charIndex]++;

		if (_selectedItem_data[_selectedItem_selected_charIndex] > '9' ||
			_selectedItem_data[_selectedItem_selected_charIndex] < '0' )
			_selectedItem_data[_selectedItem_selected_charIndex] = '0';
	}

  DrawMenu();//UNCLEAR_OLED);
}
/*KEY_CMD_TYPE_CONFIRM_CHAR*/
void OnNumNextChar(void)
{
  uint8_t length = 0;
  int i = 0;
//  sUIItem *item = 0;

//  item = &_ui_items[SELECTED_CHILD_ID];
  // get length
  for (i = 0;; i++)
  {
    if (_selectedItem_data[i] == '\0')
      break;
  }
  length = i;

  _selectedItem_selected_charIndex++;
  //跳过 小数点
  if (_selectedItem_data[_selectedItem_selected_charIndex] == '.')
    _selectedItem_selected_charIndex++;

  if (_selectedItem_selected_charIndex >= length)
    _selectedItem_selected_charIndex = 0;
  DrawMenu();//UNCLEAR_OLED);
}
/* 确认 、返回*/
void OnNumConfirm(void (*compFunct)(uint16_t cmd, char *indata, char *outdata))
{
  char outdata[20];
  compFunct(
      _ui_items_c[SELECTED_CHILD_ID].cmd,
      _selectedItem_data, outdata);

  if (outdata[0] == '1')
  {
    _path_selected_level--; //

    GetChilds(_ui_items_c[_menu_path_item[_path_selected_level - 1]
                            .childsId[_menu_path_item[_path_selected_level - 1]
                                          .selectedIndex]]
                  .id);
		/*clear*/
		OLED_CLS();
    
  }else
	{
		OLED_CLS();
		OLED_DrawTextEn(0,2,UI_ITEM_DEFAULT_SIZE_EN,outdata,0,-1,0);
		delay_tms(1800);
	}
	OLED_CLS();
	DrawMenu();//CLEAR_OLED);
}

/* check list
 * 控件******************************************************************************************/
/* check list*/
void DrawCheck(uint16_t itemindex, int x, int row, uint8_t invert, uint8_t isclear)
{
	int i=0;

	/*获取数据*/
	_UIGetData(_ui_items_c[itemindex].cmd,_selectedItem_data);
	/*绘制check*/
  DrawCheckHead(x, row, _selectedItem_data[0]);
	if(_language==UI_LANGUAGE_EN ||_ui_items_c[itemindex].zhCNlength==0)
  OLED_DrawTextEn(x + 17, row, UI_ITEM_DEFAULT_SIZE_EN, (char*)_ui_items_c[itemindex].text, invert, -1, 0);
	else if(_language==UI_LANGUAGE_CN)
	{
		for(i=0;i<_ui_items_c[itemindex].zhCNlength;i++)
			OLED_DrawTextCn(x+ 17+i*16,row,UI_ITEM_DEFAULT_SIZE_CN,_ui_items_c[itemindex].zhCNAddr[i],invert,0);
	}
}

/* check head*/
void DrawCheckHead(int x, int row, char enable)
{
  uint8_t data1[] = {0x00, 0x00, 0xFC, 0x04, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4,
                     0xF4, 0xF4, 0xF4, 0x04, 0xFC, 0x00, 0x00, 0x00, 0x00,
                     0x3F, 0x20, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F,
                     0x2F, 0x20, 0x3F, 0x00, 0x00

  };
  uint8_t data2[] = {0x00, 0x00, 0xFC, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                     0x04, 0x04, 0x04, 0x04, 0xFC, 0x00, 0x00, 0x00, 0x00,
                     0x3F, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                     0x20, 0x20, 0x3F, 0x00, 0x00

  };
  if (enable == '0' || enable== 0 )
    OLED_DrawBMP(x, row, 16, 2, data2);
  else
    OLED_DrawBMP(x, row, 16, 2, data1);
}
/* on checked*/
void OnCheckCheck(void (*compFunct)(uint16_t cmd, char *indata, char *outdata))
{
  char outdata=0;
  _selectedItem_data[0] = _selectedItem_data[0] == '0' ? '1' : '0';
  compFunct(
      _ui_items_c[SELECTED_CHILD_ID].cmd,
      _selectedItem_data, &outdata);

  if (outdata == '1')
  {
    DrawMenu();//UNCLEAR_OLED);
  }
}
/* radio list
 * 控件******************************************************************************************/
/* draw radio */
void DrawRadio(uint16_t itemindex, int x, int row, uint8_t invert, uint8_t isclear)
{
	int i=0;
	/*获取数据*/
	_UIGetData(_ui_items_c[itemindex].cmd,_selectedItem_data);
		
  DrawRadioHead(x, row, _selectedItem_data[0]);
	if(_language==UI_LANGUAGE_EN ||_ui_items_c[itemindex].zhCNlength==0)
  OLED_DrawTextEn(x + 17, row, UI_ITEM_DEFAULT_SIZE_EN, (char*)_ui_items_c[itemindex].text, invert, -1, 0);
	else if(_language==UI_LANGUAGE_CN)
	{
		for(i=0;i<_ui_items_c[itemindex].zhCNlength;i++)
			OLED_DrawTextCn(x+ 17+i*16,row,UI_ITEM_DEFAULT_SIZE_CN,_ui_items_c[itemindex].zhCNAddr[i],invert,0);
	}
}
/*radio head*/
void DrawRadioHead(int x, int row, char enable)
{
  uint8_t data1[] = {0x00, 0x00, 0xC0, 0x30, 0x08, 0x08, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x30, 0xC0, 0x00, 0x00,
                     0x00, 0x00, 0x03, 0x0C, 0x10, 0x10, 0x20, 0x20, 0x20, 0x20, 0x10, 0x10, 0x0C, 0x03, 0x00, 0x00

  };
  uint8_t data2[] = {0x00, 0x00, 0xC0, 0x30, 0x88, 0xE8, 0xE4, 0xF4, 0xF4, 0xE4, 0xE8, 0x88, 0x30, 0xC0, 0x00, 0x00,
                     0x00, 0x00, 0x03, 0x0C, 0x11, 0x17, 0x27, 0x2F, 0x2F, 0x27, 0x17, 0x11, 0x0C, 0x03, 0x00, 0x00

  };
  if (enable == '0' || enable == 0  )
    OLED_DrawBMP(x, row, 16, 2, data1);
  else
    OLED_DrawBMP(x, row, 16, 2, data2);
}
/**/
void OnRadioCheck(void (*compFunct)(uint16_t cmd, char *indata, char *outdata))
{
  char outdata=0;

  compFunct(
      _ui_items_c[SELECTED_CHILD_ID].cmd,
      _selectedItem_data, &outdata);

  if (outdata == '1')
  {
    DrawMenu();//UNCLEAR_OLED);
  }
}
/* button
 * 控件******************************************************************************************/
/*draw label*/
void DrawButton(uint16_t itemindex, int x, int row, uint8_t invert)
{
	int i = 0;

	if(_language==UI_LANGUAGE_EN ||_ui_items_c[itemindex].zhCNlength==0)
		OLED_DrawTextEn(x,row,UI_ITEM_DEFAULT_SIZE_EN,(char*)_ui_items_c[itemindex].text,invert,-1,0);
	else if(_language==UI_LANGUAGE_CN)
	{
		for(i=0;i<_ui_items_c[itemindex].zhCNlength;i++)
			OLED_DrawTextCn(x+i*16,row,UI_ITEM_DEFAULT_SIZE_CN,_ui_items_c[itemindex].zhCNAddr[i],invert,0);
	}
}
/* on checked*/
void OnButtonClick(void (*compFunct)(uint16_t cmd, char *indata, char *outdata))
{
  char outdata[64]={0};
	char *substr;
	int row=0;
	
	/*messagebox*/
	if(DrawMessage(SELECTED_CHILD_ID)==MESSAGEBOX_RESULT_OK)
	{
		OLED_CLS();
		OLED_DrawTextEn(0,0,OLED_TEXT_SIZE_6_8,"waitting...",0,-1,0);
		compFunct(
				_ui_items_c[SELECTED_CHILD_ID].cmd,
				_selectedItem_data, outdata);
		OLED_CLS();
		/*分行显示返回信息*/
		substr = strtok((char*)outdata, "\r");
		while(substr!=NULL)
		{
			OLED_DrawTextEn(0,row++,OLED_TEXT_SIZE_6_8,substr,0,-1,0);
			substr = strtok(NULL, "\r");
		}
		delay_tms(2000);
	}
	
	/*clear*/
	OLED_CLS();
	DrawMenu();//CLEAR_OLED);
}
/* message dialog
 * 控件******************************************************************************************/
int8_t DrawMessage(uint16_t itemindex)
{
	int i=0, textl = 0;
	ekey_status keystatus = KEY_STATUS_NO_KEY_PRESS;
  uint32_t key1time = 0, key2time = 0;
  uint8_t keytype = 0;
	int8_t result=-1;
	
		 //get lenth  en
  for (i = 0;; i++)
  {
    if (_ui_items_c[itemindex].text[i] == '\0')
      break;
  }
  textl = i;
	//get lenth cn
	if(_language==UI_LANGUAGE_CN)
	{
		textl=_ui_items_c[itemindex].zhCNlength==0?textl:_ui_items_c[itemindex].zhCNlength;
	}
	
	OLED_CLS();
		
	while(_messageBoxResult==-1)
	{
		delay_tms(10);
		
		if(_language==UI_LANGUAGE_EN ||_messageItems[0].zhCNlength==0)
		{
			OLED_DrawTextEn((OLED_WDITH - textl * 8) / 2,0,UI_ITEM_DEFAULT_SIZE_EN,(char*)_ui_items_c[itemindex].text,0,-1,1);/*title*/
			OLED_DrawTextEn(20,4,UI_ITEM_DEFAULT_SIZE_EN,(char*)_messageItems[0].text,_selectedMessageItemid==0?1:0,-1,0);/*ok*/
			OLED_DrawTextEn(80,4,UI_ITEM_DEFAULT_SIZE_EN,(char*)_messageItems[1].text,_selectedMessageItemid==1?1:0,-1,0);/*cancel*/
		}
		else if(_language==UI_LANGUAGE_CN)
		{
			for(i=0;i<_ui_items_c[itemindex].zhCNlength;i++)
				OLED_DrawTextCn((OLED_WDITH - textl * 16) / 2+i*16,0,UI_ITEM_DEFAULT_SIZE_CN,_ui_items_c[itemindex].zhCNAddr[i],0,1);/*title*/
			for(i=0;i<_messageItems[0].zhCNlength;i++)
				OLED_DrawTextCn(20+i*16,4,UI_ITEM_DEFAULT_SIZE_CN,_messageItems[0].zhCNAddr[i],_selectedMessageItemid==0?1:0,0);/*ok*/
			for(i=0;i<_messageItems[1].zhCNlength;i++)
				OLED_DrawTextCn(80+i*16,4,UI_ITEM_DEFAULT_SIZE_CN,_messageItems[1].zhCNAddr[i],_selectedMessageItemid==1?1:0,0);/*cancel*/
		}
		
		//获取按键状态、时间
		KeyScan(&keystatus, &key1time, &key2time);
		//无按键则退出
		if (keystatus == KEY_STATUS_NO_KEY_PRESS)
		{
			/*长时间无按键则退回mainlayer*/
			if(_path_selected_level>UI_MAIN_LAYER_ID)
			{
				if((xTaskGetTickCount()-_menuLastClickTime)>pdMS_TO_TICKS(_menuTimeOut*1000))
				{
					_path_selected_level=UI_MAIN_LAYER_ID;
					OLED_CLS();
					/*清除状态*/
					_selectedMessageItemid=0;
					_messageBoxResult=-1;
					return -1;
				}
			}
			continue;
		} 
    //否则，获取按键操作类型
		keytype = GetKeyCmdType(ITEM_TYPE_MESSAGE_BOX_ITEM,keystatus, key1time, key2time);
		/*切换显示*/
		if(keytype==KEY_EVENT_TYPE_MESSAGEBOX_CIRCLE)
		{
			_selectedMessageItemid=_selectedMessageItemid==0?1:0;
		}
		else if(keytype==KEY_EVENT_TYPE_MESSAGEBOX_CONFIRM)
		{
			_messageBoxResult=_selectedMessageItemid;
		}	
	}
	/*选择结果*/
	result=_messageBoxResult==0?MESSAGEBOX_RESULT_OK:MESSAGEBOX_RESULT_CANCEL;
	/*清除状态*/
	_selectedMessageItemid=1;
	_messageBoxResult=-1;
	
	return result;
}
/* DI config
 * 控件******************************************************************************************/
/* DI config*/
void DrawDIConfig(uint16_t itemindex, int x, int row, uint8_t isclear)
{
	int i=0;
	char diid[16]={0};
	int dinum=0;
	/*获取数据*/
	//memset(_selectedItem_data,0,sizeof(_selectedItem_data));
	//第一次加载，之后不加载
	if(_selectedItem_data[0]==0)
		_UIGetData(_ui_items_c[itemindex].cmd,_selectedItem_data);//data "1111111111111111"
	dinum=strlen(_selectedItem_data);//数量由输入字符串长度控制
	//标题
	if(_language==UI_LANGUAGE_EN ||_ui_items_c[itemindex].zhCNlength==0)
		OLED_DrawTextEn(x + 17, row, UI_ITEM_DEFAULT_SIZE_EN, (char*)_ui_items_c[itemindex].text, 0, -1, 0);
	else if(_language==UI_LANGUAGE_CN)
	{
		for(i=0;i<_ui_items_c[itemindex].zhCNlength;i++)
			OLED_DrawTextCn(x+ 30+i*16,row,UI_ITEM_DEFAULT_SIZE_CN,_ui_items_c[itemindex].zhCNAddr[i],0,0);
	}
	
	for(i=0;i<dinum;i++)
	{
		sprintf(diid,"%02d",i+1);
		if(i==_selectedItem_selected_charIndex)
			OLED_DrawTextEn(i%8*16,i/8*3+2,OLED_TEXT_SIZE_6_8,diid,1,-1,0);
		else
			OLED_DrawTextEn(i%8*16,i/8*3+2,OLED_TEXT_SIZE_6_8,diid,0,-1,0);
		DrawDIConfigHead(i%8*16, i/8*3+3,_selectedItem_data[i]);
	}
}

/* di config head*/
void DrawDIConfigHead(int x, int row, char enable)
{
  uint8_t data1[] = {0x00, 0x00, 0xFC, 0x04, 0xF4, 0xF4, 0xF4, 0xF4, 0xF4,
                     0xF4, 0xF4, 0xF4, 0x04, 0xFC, 0x00, 0x00, 0x00, 0x00,
                     0x3F, 0x20, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F,
                     0x2F, 0x20, 0x3F, 0x00, 0x00

  };
  uint8_t data2[] = {0x00, 0x00, 0xFC, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                     0x04, 0x04, 0x04, 0x04, 0xFC, 0x00, 0x00, 0x00, 0x00,
                     0x3F, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                     0x20, 0x20, 0x3F, 0x00, 0x00

  };
  if (enable == '0' || enable== 0 )
    OLED_DrawBMP(x, row, 16, 2, data2);
  else
    OLED_DrawBMP(x, row, 16, 2, data1);
}

void OnDIConfigNextBit(void)
{
	uint8_t length = strlen(_selectedItem_data);
  _selectedItem_selected_charIndex++;
  if (_selectedItem_selected_charIndex >= length)
    _selectedItem_selected_charIndex = 0;
  DrawMenu();//UNCLEAR_OLED);
}

void OnDIConfigChangeBit(void (*compFunct)(uint16_t cmd, char *indata, char *outdata))
{
	if (_selectedItem_data[_selectedItem_selected_charIndex] >= 0x31)
		_selectedItem_data[_selectedItem_selected_charIndex] = 0x30;
	else
		_selectedItem_data[_selectedItem_selected_charIndex] = 0x31;

  DrawMenu();//UNCLEAR_OLED);
}
void OnDIConfigConfirm(void (*compFunct)(uint16_t cmd, char *indata, char *outdata))
{
  char outdata=0;
	
	compFunct(_ui_items_c[SELECTED_CHILD_ID].cmd,
      _selectedItem_data, &outdata);
  if (outdata == '1')
  {
    DrawMenu();//UNCLEAR_OLED);
  }
}
/* signal
 * 控件******************************************************************************************/
void DrawSignal(uint8_t signal)
{
  int i = 0;
  uint8_t data[] = {0x40, 0x00, 0x60, 0x00, 0x70, 0x00, 0x78, 0x00, 0x7c};
  //uint8_t data1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  OLED_DrawTextEn(5, 0, OLED_TEXT_SIZE_6_8, "T", 0, -1, 0);
  for (i = 0; i < 5; i++)
  {
    if (signal <= i)
    {
      data[i * 2] = 0x00;
    }
  }
  OLED_DrawBMP(15, 0, 9, 1, data);
  //OLED_DrawBMP(25, 0, 1, 8, data1);
}
/* power
 * 控件******************************************************************************************/
void DrawPower(uint8_t power)
{
  uint8_t data[] = {0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e,
                    0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x3c};
  int i = 0;
  for (i = 0; i < 10; i++)
  {
    if (power <= (i / 2))
    {
      data[1 + i] = 0x42;
    }
    else
    {
      data[1 + i] = 0x7e;
    }
  }
  OLED_DrawBMP(114, 0, 13, 1, data);
}

/**/
void DrawDILight(int x, int row, char enable)
{
  uint8_t data1[] = {0x00, 0x00, 0xC0, 0x30, 0x08, 0x08, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x30, 0xC0, 0x00, 0x00,
                     0x00, 0x00, 0x03, 0x0C, 0x10, 0x10, 0x20, 0x20, 0x20, 0x20, 0x10, 0x10, 0x0C, 0x03, 0x00, 0x00

  };
  uint8_t data2[] = {0x00, 0x00, 0xC0, 0x30, 0x88, 0xE8, 0xE4, 0xF4, 0xF4, 0xE4, 0xE8, 0x88, 0x30, 0xC0, 0x00, 0x00,
                     0x00, 0x00, 0x03, 0x0C, 0x11, 0x17, 0x27, 0x2F, 0x2F, 0x27, 0x17, 0x11, 0x0C, 0x03, 0x00, 0x00

  };
  if (enable == '0')
    OLED_DrawBMP(x, row, 6, 2, data1);
  else
    OLED_DrawBMP(x, row, 16, 2, data2);
}
/*Draw logo*/
void DrawLogo(void)
{
	OLED_DrawLogo(0, 2);
//	delay_tms(2000);
}

