
#ifndef LADDER_H_
#define	LADDER_H_

#include "string.h"
#include "stdint.h"
#include "rs485.h"





/* 
 *  �Ƿ����ݿع���
 */
#if (DEVICE_FUNCTION&FIRMWARE_FLAG_TIKONG)
#define USE_LADDER	1
#else
#define USE_LADDER	0	 
#endif


//ʹ���ݿس���

#define	TIBOSHI				


//Э���ֽ�ƫ�������ֽ� Э��˳�� ����ID->�����ֶ�ID->�û�¥����Ϣ->�û�������Ϣ->�û�¥��Ȩ��
#define OPEN_ID_OFFSET      0
#define OPEN_ID_SIZE        1

#define DATA_TYPE_OFFSET    OPEN_ID_OFFSET+OPEN_ID_SIZE
#define DATA_TYPE_SIZE      1

#define USER_FLOOR_OFFSET   DATA_TYPE_OFFSET+DATA_TYPE_SIZE
#define USER_FLOOR_SIZE     3


#define USER_ROOM_OFFSET    USER_FLOOR_OFFSET+USER_FLOOR_SIZE
#define USER_ROOM_SIZE      4

#define LIMIT_FLOOR_OFFSET  USER_ROOM_OFFSET+USER_ROOM_SIZE
#define LIMIT_FLOOR_SIZE    16

#define INSTALL_CAR         0 
#define INSTALL_UINT        1

typedef enum
{
	LADDER_USER_POWER =0,
	LADDER_ADMIN_POWER,
}ladderPowerEnum;

typedef struct
{
	uint16_t	userFloor;
	uint16_t	userRoom;
    uint8_t		cardId[4];
	uint8_t		floor[8];
}LadderUserMsgType;



void ladder_set_form_app( uint8_t *msg);
void ladder_set_form_card( uint8_t* data  , ladderPowerEnum powerType );


extern uint8_t  get_control_data(LadderUserMsgType *pst , uint8_t *respone);

#endif








