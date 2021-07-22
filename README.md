# oledmenu
小尺寸点阵屏多级配置菜单（2按键）
功能：主屏显示、密码、超级密码、多级菜单、菜单控件类型、按键事件、菜单超时返回、


使用示例：
/*menu item 编辑菜单*/
static uint8_t _ui_items_enable[MAX_ITEM_NUM]={0};
static const sUIItemConst _ui_items_c[] = {
//id,parent,itemType,textEN,CMD,cnlength,CNaddr1,CNaddr2,CNaddr3,CNaddr4,CNaddr5,
{0,0,ITEM_TYPE_PASS_INPUT,"Password",0,2,45,46,12,13,0},//密码
{1,0,ITEM_TYPE_PASS_INPUT,"SuperPass",0,0,0,0,0,0,0},//超级密码,不做显示，仅作为分支节点使用,,
/*工厂设置参数*/
{2,-2, ITEM_TYPE_BUTTON,"param backup",7012,4,51,52,73,70,0},//backup---
{3,-2, ITEM_TYPE_MENU_LABLE,"device type",0,4,12,51,25,26,0},//device type---
{4,3, ITEM_TYPE_RADIO,"master",7001,2,64,66,0,0,0},//
{5,3, ITEM_TYPE_RADIO,"slave",7002,2,65,66,0,0,},//
{6,-2, ITEM_TYPE_MENU_LABLE,"Up ComType",0,4,16,67,82,83,0},//uplink---
{7,6, ITEM_TYPE_RADIO,"RS485",7003,0,0,0,0,0,0},//
{8,6, ITEM_TYPE_RADIO,"Lora",7004,0,0,0,0,0,0},//
{9,6, ITEM_TYPE_RADIO,"LTE",7005,0,0,0,0,0,0},//
{10,6, ITEM_TYPE_RADIO,"WIFI",7016,0,0,0,0,0,0},//
{11,-2, ITEM_TYPE_MENU_LABLE,"Slave ComType",0,4,17,67,82,83,0},//slave---
{12,11, ITEM_TYPE_RADIO,"NONE",7006,0,0,0,0,0,0},//
{13,11, ITEM_TYPE_RADIO,"RS485",7007,0,0,0,0,0,0},//
{14,11, ITEM_TYPE_RADIO,"Lora",7008,0,0,0,0,0,0},//
{15,-2, ITEM_TYPE_MENU_LABLE,"show logo",0,4,74,75,40,76,0},//show logo---
{16,15, ITEM_TYPE_RADIO,"hide",7010,2,91,92,0,0,0},//
{17,15, ITEM_TYPE_RADIO,"show",7011,2,89,90,0,0,0},//
{18,-1, ITEM_TYPE_MENU_LABLE,"COM",0,4,10,43,12,13,0},//com------------------------------
{19,18, ITEM_TYPE_MENU_LABLE,"Uplink",0,2,16,67,0,0,0},//uplink----------
{20,19, ITEM_TYPE_MENU_LABLE,"IP",0,0,0,0,0,0,0},//ip---
{21,20, ITEM_TYPE_NUM,"IPInput",5001,0,0,0,0,0,0},//
{22,19, ITEM_TYPE_MENU_LABLE,"PORT",0,2,33,34,0,0,0},//Port---
{23,22, ITEM_TYPE_NUM,"PORTInput",5002,2,33,34,0,0,0},//
{24,19, ITEM_TYPE_MENU_LABLE,"Net Number",0,3,68,69,24,0,0},//netnumber--
{25,24, ITEM_TYPE_NUM,"Net Number",5003,3,68,69,24,0,0},//
{26,19, ITEM_TYPE_MENU_LABLE,"Baud",0,3,127,128,129,0,0},//baud---
{27,26, ITEM_TYPE_RADIO,"4800",5013,0,0,0,0,0,0},
{28,26, ITEM_TYPE_RADIO,"9600",5014,0,0,0,0,0,0},
{29,26, ITEM_TYPE_RADIO,"57600",5015,0,0,0,0,0,0},
{30,26, ITEM_TYPE_RADIO,"115200",5016,0,0,0,0,0,0},
{31,19, ITEM_TYPE_MENU_LABLE,"Parity",0,3,130,131,20,0,0},//parity--
{32,31, ITEM_TYPE_RADIO,"none",5017,0,0,0,0,0,0},//
{33,31, ITEM_TYPE_RADIO,"odd",5018,0,0,0,0,0,0},//
{34,31, ITEM_TYPE_RADIO,"even",5019,0,0,0,0,0,0},//
{35,18, ITEM_TYPE_MENU_LABLE,"Slave",0,2,17,67,0,0,0},//slave----------
{36,35, ITEM_TYPE_MENU_LABLE,"Net Number",0,3,68,69,24,0,0},//net number---
{37,36, ITEM_TYPE_NUM,"Net Number",5004,3,68,69,24,0,0},//
{38,35, ITEM_TYPE_MENU_LABLE,"Baud",0,3,127,128,129,0,0},//baud--
{39,38, ITEM_TYPE_RADIO,"4800",5005,0,0,0,0,0,0},
{40,38, ITEM_TYPE_RADIO,"9600",5006,0,0,0,0,0,0},
{41,38, ITEM_TYPE_RADIO,"57600",5007,0,0,0,0,0,0},
{42,38, ITEM_TYPE_RADIO,"115200",5008,0,0,0,0,0,0},
{43,35, ITEM_TYPE_MENU_LABLE,"Parity",0,3,130,131,20,0,0},//parity--
{44,43, ITEM_TYPE_RADIO,"none",5009,0,0,0,0,0,0},//
{45,43, ITEM_TYPE_RADIO,"odd",5010,0,0,0,0,0,0},//
{46,43, ITEM_TYPE_RADIO,"even",5011,0,0,0,0,0,0},//
{47,35, ITEM_TYPE_MENU_LABLE,"SlaveNumber",0,4,65,66,70,71,0},//slave number---
{48,47, ITEM_TYPE_NUM,"SlaveNumber",5012,4,65,66,70,71,0},//
{49,-1, ITEM_TYPE_MENU_LABLE,"SYSTEM",0,4,35,36,12,13,0},//system----------------------------------
{50,49, ITEM_TYPE_MENU_LABLE,"SN",0,4,12,51,44,24,0},//sn---
{51,50, ITEM_TYPE_NUM,"SNInput",6001,4,12,51,44,24,0},//
{52,49, ITEM_TYPE_MENU_LABLE,"SlaveID",0,4,65,66,122,123,0},//slaveid---
{53,53, ITEM_TYPE_NUM,"SlaveIDInput",6002,4,65,66,122,123,0},//
{54,49, ITEM_TYPE_MENU_LABLE,"Interval",0,4,16,124,125,126,0},//interval---
{55,54, ITEM_TYPE_NUM,"Interval",6003,4,19,20,156,157,0},//
{56,49, ITEM_TYPE_MENU_LABLE,"Language",0,2,87,88,0,0,0},//language---
{57,56, ITEM_TYPE_RADIO,"zh-CN",6004,3,84,86,133,0,0},//
{58,56, ITEM_TYPE_RADIO,"English",6005,0,85,86,63,0,0},//
{59,49, ITEM_TYPE_MENU_LABLE,"modifyPass",0,4,135,136,45,46,0},//pass---
{60,59, ITEM_TYPE_NUM,"Pass",5020,3,134,45,46,0,0},//淇敼瀵嗙爜
{61,49, ITEM_TYPE_BUTTON,"param Recover",6006,4,37,38,39,40,0},//recover---
{62,49, ITEM_TYPE_BUTTON,"Reset",6007,4,12,51,41,42,0},
};

/*key task 按键事件监控*/
static void taskKey(void *pvParameters)
{
	/*key gpio初始化*/
	KeyInit(_KeySemaphore);
	for (;;)
	{
		/* 等待所有任务发来事件标志 */
		KeyWait();
		//获取按键状态、时间
		KeyScan(&_KeyStatus, &_Key1HoldingTime, &_Key2HoldingTime);
		/*互斥访问控制-非阻塞*/
		xSemaphoreTake(_KeyBusySemaphore, portMAX_DELAY);
		if (_KeyStatus != KEY_STATUS_NO_KEY_PRESS)
		{
			UIOnKeyPress(_KeyStatus, _Key1HoldingTime, _Key2HoldingTime);
		}
		KeyReset();
		/*互斥访问控制-释放*/
		xSemaphoreGive(_KeyBusySemaphore);
	}
}
/*ui task 界面刷新任务*/
static void taskUI(void *pvParameters)
{
	/*init ui*/
	UIInit(&GetDataCallback,&DataUpdataCallback);
	UISetLanguage(_PROJECT_cfg.Language);
	UIStart(_PROJECT_cfg.ShowLogo);

	for (;;)
	{
		xSemaphoreTake(_KeyBusySemaphore, portMAX_DELAY);
		/*菜单项目筛选*/
		SetUIItemsEnable();
		//并刷新主界面显示
		UIMainLayerRefersh(2); 
		xSemaphoreGive(_KeyBusySemaphore);
		delay_tms(40);
		Wwdt_Feed();
	}
}

/* GetDataCallback 菜单数据读取 */
void GetDataCallback(uint16_t cmd, char *outdata)
{
	//char str[20] = {0};
	switch (cmd)
	{
	case 0xFFFF: //ITEM_TYPE_PASS_INPUT,UI_ITEM_ENABLE,"Password"
		strcpy(outdata, "0000");
		break;
	case 5001:
		sprintf(outdata, "%03d.%03d.%03d.%03d", _PROJECT_cfg.hreg.UP_IP[0]>>8&0xFF, 
	    _PROJECT_cfg.hreg.UP_IP[0]&0xFF, _PROJECT_cfg.hreg.UP_IP[1]>>8&0xFF, _PROJECT_cfg.hreg.UP_IP[1]&0xFF);
	    break;
    //...

    //...
    }
}

/* DataUpdataCallback 菜单元素对应数据更新*/
void DataUpdataCallback(uint16_t cmd, char *indata, char *outdata)
{
	char *str = 0;
	int 	indataLen=strlen(indata);
	
	switch (cmd)
	{
	case 0xFFFF://0,ITEM_TYPE_PASS_INPUT,UI_ITEM_ENABLE,"""Password""//密码",
		if(strcmp(indata, _PROJECT_cfg.Password) == 0)
			outdata[0]='1';
		else if(strcmp(indata, _PROJECT_cfg.SuperPassword) == 0)
			outdata[0]='2';
		else
			outdata[0]='0';
	break;
	case 5001://472, ITEM_TYPE_NUM,UI_ITEM_ENABLE,"IPInput""//"
		str = strtok(indata, ".");
		//str = strtok(NULL, str);
		_PROJECT_cfg.hreg.UP_IP[0] = atoi(str)<<8;
		str = strtok(NULL, ".");
		_PROJECT_cfg.hreg.UP_IP[0] |= atoi(str);
		str = strtok(NULL, ".");
		_PROJECT_cfg.hreg.UP_IP[1] = atoi(str)<<8;
		str = strtok(NULL, ".");
		_PROJECT_cfg.hreg.UP_IP[1] = atoi(str);
		outdata[0] = '1';
		_PROJECT_status.UP_CfgChanged=1;
	break;
    }
    //存储参数
    SaveProfile((uint8_t *)&_PROJECT_cfg, sizeof(_PROJECT_cfg));
}
