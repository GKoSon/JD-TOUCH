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
*������ tag_fukai_card_process
*������������ �� ������Ƭ���ݽ���
*�������� ��tag ������Ƭ��Ϣ
*��������ֵ ��������
*���� �� ���ֺ�
*������������ ��2017��12��27��
*�����޸����� ����
*�޸��� �� ��
*�޸�ԭ�� �� ��
*�汾 ��1.0
*��ʷ�汾 ����
*******************************************************************************/
uint8_t tag_fukai_card_process( tagBufferType *tag)
{
     log_err("������˾��Ƭ���ͣ���֧�ִ�Ȩ�޽���, POWER = %d.\n" , tag->tagPower);
     return TAG_NO_SUPPORT;

}