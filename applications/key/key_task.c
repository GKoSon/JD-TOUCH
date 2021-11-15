#include "key_task.h"
#include "unit.h"
#include "beep.h"
#include "sysCfg.h"
#include "timer.h"
#include "open_door.h"
#include "config.h"
#include "tempwd.h"
#include "bsp_rtc.h"
#include "open_log.h"
#include "permi_list.h"
#include "err_log.h"
#include "backlight.h"
#include "open_log.h"
#include "tslDataHand.h"

static xTaskHandle 	keyTask;
static touchKeyType	touchKey;
static void touch_key_clear( void );

uint8_t keyTimerHandle = 0xFF;
blinkParmType beepLengthErr=
{
    .mode = BLINK_OPEN_NOW,
    .openCnt = 4,
    .openTime = 50,
    .closeTime = 50,
    .delayTime = 0,
};



keyValueMaType keyValueMap[10]=
{
  {KEY_0 , 0},
  {KEY_1 , 1},
  {KEY_2 , 2},
  {KEY_3 , 3},
  {KEY_4 , 4},
  {KEY_5 , 5},
  {KEY_6 , 6},
  {KEY_7 , 7},
  {KEY_8 , 8},
  {KEY_9 , 9},
};



static void key_timer_isr ( void )
{
	touch_key_clear();
    log(DEBUG,"按键超时,清空按键缓存\n");
}

static void system_key_handle(keyTaskType *key)
{
	switch(key->keyValue)
	{
	case KEY_1_SEC:
	{
		 tag_config_enable(TRUE);
		 beep.write(BEEP_NORMAL);
                 
         log(DEBUG,"使能安装工刷卡\n");

	}break;
	case KEY_8_SEC:
	{
        tag_config_enable(FALSE);
		beep.write_base(&beepRestore);
		set_clear_flash(FLASH_ALL_DATA);
	}break;
	default:log_err("按键键值错误，VALUE=%d\n" ,key->keyValue);
	}
    
    
}

static void fun_key_handle(keyTaskType *key)
{
	switch(key->keyValue)
	{
	case OPEN_DOOR_KEY:
	{
		openlogDataType	logData;
		
		memset(&logData , 0x00 , sizeof(openlogDataType));
		open_door();
		beep.write(BEEP_NORMAL);
		logData.type = OPENLOG_FORM_KEY;
		
		log(ERR,"门内开门成功\n");

//journal.save_log(&logData);  一句话使能log上报
        
	}break;

	default:log_err("按键键值错误，VALUE=%d\n" ,key->keyValue);
	}


}


static void touch_key_clear( void )
{
	memset(&touchKey  , 0x00 , sizeof(touchKeyType));
	timer.stop(keyTimerHandle);
}


static uint8_t touch_key_read_value(touchKeyEnum keyEnum )
{
    for(uint8_t i = 0 ; i < sizeof (keyValueMap) / sizeof (keyValueMap[0]); i++)
    {
        if(keyEnum == keyValueMap[i].key)
        {
            return keyValueMap[i].value;
        }
    }
    
    return 0xFF;
}

static void touch_key_handle(keyTaskType *key)
{
    //backlight_set(BACKLIGHT_ON);
  
    key->keyValue = cy3116_read_key();
    if( key->keyValue == KEY_INIT)
    {
        return ; //无效按键
    }
    timer.start(keyTimerHandle);
	switch(key->keyValue)
	{
		case KEY_0:case KEY_1: case KEY_2: case KEY_3: case KEY_4:
		case KEY_5:case KEY_6: case KEY_7: case KEY_8: case KEY_9:
		{
			if( touchKey.length < PASSWORD_MAX_LENGTH)
			{
				touchKey.buffer[touchKey.length++] = touch_key_read_value((touchKeyEnum)key->keyValue);
                                beep.write(BEEP_NORMAL);
			}
			else
			{
				log(DEBUG,"按键输入长度太长\n");
				log_arry(DEBUG,"输入按键键值 = " ,touchKey.buffer , touchKey.length);
				touch_key_clear();
				beep.write_base(&beepLengthErr);
			}
		}break;

		case KEY_DEL:
		{
			touch_key_clear();
			beep.write(BEEP_NORMAL);
		}break;

		case KEY_ENT:
		{
			log_arry(DEBUG,"输入按键键值 = " ,touchKey.buffer , touchKey.length);
                        uint32_t time = rtc.read_stamp();
			switch(touchKey.length)
			{
				case FIXED_PASSWORD_LENGTH:
				{
                                      
					uint8_t *userPwd , *pairPwd;
					uint8_t pwd[3];
                                        openlogDataType	logData;
                    
					log(DEBUG,"校验固定密码\n");

					pwd[0] = touchKey.buffer[0]<<4|touchKey.buffer[1];
					pwd[1] = touchKey.buffer[2]<<4|touchKey.buffer[3];
					pwd[2] = touchKey.buffer[4]<<4|touchKey.buffer[5];
					config.read(CFG_PAIR_PWD , (void **)&pairPwd );
					config.read(CFG_USER_PWD , (void **)&userPwd );

					if((aiot_strcmp(pwd , pairPwd , 3) == TRUE )||(aiot_strcmp(pwd , userPwd , 3) == TRUE ))
					{
						open_door();
						memset(&logData , 0x00 , sizeof(openlogDataType));
						beep.write(BEEP_NORMAL);
						logData.type = OPENLOG_FORM_PWD;
						logData.length = 3;
						memcpy(logData.data , pwd , 3);
                                                memcpy(&logData.data[6] , &time , 4);
						journal.save_log(&logData);
					}
					else
					{
						beep.write(BEEP_ALARM);
					}
				}break;
				case TEMPORARY_PASSWORD_LENGTH:
				{
                                    int32_t tempPwd =0;
                                    uint32_t TimeA=0,TimeB=0;
                                    tempwdType getPwd;
                                    openlogDataType	logData;
                                    
                                    tempPwd = touchKey.buffer[0]<<12|touchKey.buffer[1]<<8|touchKey.buffer[2]<<4|touchKey.buffer[3];
                                    log(DEBUG,"校验临时密码 %x \n" , tempPwd);
                                    

                                    memcpy((uint8_t *)&TimeA ,(uint8_t *)&getPwd.temp , 4);//起始事件TIMEA
                                    TimeB = getPwd.time;

                                    printf("TimeNow = %d 【timea = %d----timeb】 = %d\r\n",time,TimeA ,TimeB);
                                    
                                    if( tempwd.find(tempPwd , &getPwd) != PWD_NULL_ID)
                                    {
                                        if(time > TimeA    &&      time < TimeB)//以前 没有TIME A
                                        {
                                            open_door();
                                            memset(&logData , 0x00 , sizeof(openlogDataType));
                                            beep.write(BEEP_NORMAL);
                                            logData.type = OPENLOG_FORM_PWD;
                                            logData.length = 4;
                                            memcpy(logData.data , (uint8_t *)touchKey.buffer , PASSWORD_MAX_LENGTH);
                                            memcpy(&logData.data[6] , &time , 4);
                                            journal.save_log(&logData);
                                        }
                                        else
                                        {
                                            log(INFO,"临时密码不在有效期\n");
                                            beep.write(BEEP_ALARM);
                                        }
                                        tempwd.del(tempPwd);
                                    }
                                    else
                                    {
                                        log(DEBUG,"没有相同的临时密码,开门失败\n");
                                        beep.write(BEEP_ALARM);
                                    }
                    
				}break;
				default :
				{
					log(DEBUG,"密码长度错误, length = %d.\r\n" , touchKey.length);
					beep.write_base(&beepLengthErr);
				}break;
			}
			touch_key_clear();

		}break;
		default : touch_key_clear(); break;
	}
}

static void key_handle(keyTaskType *key)
{
	switch(key->cmd)
	{
	case SYS_KEY:
	{
        system_key_handle(key);
	}break;
	case TOUCH_KEY:
	{
       // touch_key_handle(key);
	}break;
	case FUN_KEY:
	{
        fun_key_handle(key);
	}break;
	default:log_err("按键命令错误，CMD=%d\n" ,key->cmd);
	}
}

static void key_task( void const *pvParameters)
{

    keyTaskType key;


    configASSERT( ( ( unsigned long ) pvParameters ) == 0 );
    
    keyTimerHandle = timer.creat(15000 , FALSE , key_timer_isr);
    touch_key_clear();
    memset(&key,0x00,sizeof(keyTaskType));
    
    while(1)
    { 
       // beep.write(1);sys_delay(500);
        if(xQueueReceive( xKeyQueue, &key, 1000 ) == pdTRUE)
        {
            key_handle(&key);
            memset(&key,0x00,sizeof(keyTaskType));
        }
        
        task_keep_alive(TASK_KEY_BIT);

    }
}


void creat_key_task( void )
{

    osThreadDef( key, key_task , osPriorityHigh, 0, configMINIMAL_STACK_SIZE*3);
    keyTask = osThreadCreate(osThread(key), NULL);
    configASSERT(keyTask);
}



