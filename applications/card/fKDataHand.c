#include "tslDatahand.h"
#include "open_log.h"
#include "open_door.h"
#include "magnet.h"
#include "bsp_rtc.h"
#include "unit.h"
#include "ladder.h"
#include "beep.h"
#include "fkDataHand.h"



/******************************************************************************
*函数名 tag_fukai_card_process
*函数功能描述 ： 富凯卡片数据解析
*函数参数 ：tag 富凯卡片信息
*函数返回值 ：处理结果
*作者 ： 汪林海
*函数创建日期 ：2017年12月27日
*函数修改日期 ：无
*修改人 ： 无
*修改原因 ： 无
*版本 ：1.0
*历史版本 ：无
*******************************************************************************/
uint8_t tag_fukai_card_process( tagBufferType *tag)
{
     log_err("富凯公司卡片类型，不支持此权限解析, POWER = %d.\n" , tag->tagPower);
     return TAG_NO_SUPPORT;

}