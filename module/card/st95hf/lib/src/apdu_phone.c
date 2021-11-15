
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stdlib.h"
#include "sysCfg.h"
#include "apdu_phone.h"
#include "apdu_drv.h"
#include "drv95HF.h"
#include "unit.h"
#include "Hwconfig.h"
#include "relay.h"
#include "phoneTag.h"

extern ISO14443A_CARD 	ISO14443A_Card;

#define CMD_WRITE_MODE (0)
#define CMD_READ_MODE  (1)

uint8_t detectionPhoneResaultCnt = 0;                   //������
uint8_t detectionPhoneResault = PHONE_DETECTION_FAIL;   //�ж��Ƿ��⵽�ֻ�
uint8_t readPhoneResault = FALSE;                       //�ж��Ƿ���APPͨѶ�ɹ�
uint8_t writeOrReadCmd = CMD_WRITE_MODE;                //�������ͣ�ģʽ�л�
uint32_t openTimeFormPhone = 0;      //����ʱ��  

uint8_t returnApduData[6]={0x0A,0x0A,0x0A,0x0A,0x0A,0x0A};  //Э������ֶ�


//App��AID���̶��� A4 04λselect����
uint8_t select_app_name[] ={0x02,0X00,0XA4,0X04,0X00,0X08,'t','e','r','m','i','n','u','s',0x28};
uint8_t const gucApplicationAID[]="terminus";


/*
*ST95�������ݴ�ӡ���ڶ�λ�ǳ���
*/
void PrintApduMessage(uint8_t *pucData)
{
    uint8_t i = 0 ; 
    if( pucData[1]+2 > 50)
    {
        apdu_log(DEBUG,"printf buff is too long.\r\n");
        return ;
    }
    for( i = 0 ;  i < pucData[1]+2; i++)
    {
        printf("0x%x " , pucData[i]);
    }
    printf("\r\n");

}

/*
*�ֽڱȽϺ���
*/
uint8_t ByteCompare(uint8_t *str , uint8_t *pst , uint8_t len)
{
    unsigned char i = 0 ; 
    
    if(( str == NULL) ||( pst == NULL) || ( len > 50))
    {
        apdu_log(DEBUG,"com pare is null , or too long .\r\n");
    }
    
    for( i = 0 ; i < len ; i++)
    {
        if( str[i] != pst[i])
        {
            return 0;
        }
    }
    
    return 1 ;
  
}

/*
*����ST95������Ϣ�ӿ�
*/
uint8_t DrvWriteConfig(uint8_t *pucData , uint8_t ucLength , uint8_t *pucReadBuff)
{
    if( PCD_SendRecv(ucLength,pucData,pucReadBuff)!= PCD_SUCCESSCODE)
    {
        return FALSE;
    }
    
    return TRUE;
}
/*
*����ST95���ݽӿ�
*/
uint8_t DrvWriteData(uint8_t *pucData , uint8_t ucLength , uint8_t *pucReadBuff)
{
    if( PCD_SendRecv(ucLength,pucData,pucReadBuff)!= PCD_SUCCESSCODE)
    {
        PrintApduMessage(pucReadBuff);
        return FALSE;
    }
    
    if( pucReadBuff[CR95HF_LENGTH_OFFSET] < ST95_NDEF_DATA_OFFSET_LEN )
    {
        PrintApduMessage(pucReadBuff);
        return FALSE;
    }
    
    return TRUE;
}
/*
*ѡ��Э�飬���÷������
*/
uint8_t DeviceSelectProtocol( void )
{
    uint8_t select_protocol[]={0x02,0x04,0x02,0x00,0x01,0xA0};
    uint8_t config[]={0x09,0x04,0x3a,0x00,0x5F,0x04};
    uint8_t gain[]={0x09,0x04,0x68,0x01,0x01,0xdF};
    uint8_t pucDataRead[256]={0x00};
    uint8_t ucErrCode =0;
    //select protocol
    if( (ucErrCode = halCheckSendRecv( select_protocol,pucDataRead)) != RESULTOK)
    {
        apdu_log(DEBUG,"Select protocil error code = %x .\r\n" , ucErrCode);
        PrintApduMessage(pucDataRead);
        return SELECT_PROTOL_ERR;
    }
    if( (ucErrCode = halCheckSendRecv( config,pucDataRead)) != RESULTOK)
    {
        apdu_log(DEBUG,"config error code = %x .\r\n" , ucErrCode);
        PrintApduMessage(pucDataRead);
        return CONFIG_DEVICE_ERR;
    }
    if( ( ucErrCode= halCheckSendRecv( gain,pucDataRead)) != RESULTOK)
    {
        apdu_log(DEBUG,"config error code = %x .\r\n" , ucErrCode);
        PrintApduMessage(pucDataRead);
        return SET_GAIN_ERR;
    }
    
    return SELECT_PROTOCOL_SUCCESS;
}

uint8_t st95Iso14443aSetPPS( void )
{
	uint8_t pps[]={0xD0,0x11,0x00,0x28};
	uint8_t pucDataRead[256]={0x00};
		
	memset(pucDataRead , 0x00 , 256);
	if(DrvWriteConfig(pps , sizeof(pps),pucDataRead) == TRUE )
	{
		return TRUE;
	}
	
	return FALSE;
}

/*
*��ȡ�ֻ�Ӳ����UUID
*/
uint8_t SearchPhoneHalUUID( void )
{

    uint8_t reqa[]={0x26,0x07};
    uint8_t anticol[]={0x93,0x20,0x08};
    uint8_t select1[]={0X93,0X70,0x00,0X00,0X00,0X00,0X00,0X28};
    uint8_t rats[]={0xE0,0x50,0x28};
    uint8_t pps[]={0xD0,0x11,0x00,0x28};

    uint8_t pucDataRead[256]={0x00};
    //uint8_t ucErrCode =0;
    uint8_t ucTagCheck = 0;

    //read uuid form phone nfc ic
    //Reqa����سɹ�����ʾ���ֻ�Ӳ���������ӳɹ�
    if(DrvWriteConfig(reqa , sizeof(reqa),pucDataRead) == TRUE )
    {
        detectionPhoneResaultCnt = 0;    //ʶ���ֻ�������ֻ����ӶϿ��жϵļ���
        //����ֻ�Ӳ�����ӳɹ�����ʱ2�뱨��
        if(detectionPhoneResault == PHONE_DETECTION_SUCCESS)
        {
            //BeepLedOpen(2 , 3 , 2000);
        }
        //�����APP�����ɹ�����ֱ�ӷ��أ������һֱ����select����ֻ���һֱ����ʾ��
        if( readPhoneResault == TRUE)
        {
            return OPEN_SUCCESS_ERR;
        }
        if(DrvWriteConfig(anticol , sizeof(anticol),pucDataRead) == TRUE )
        {
            ucTagCheck = pucDataRead[OFFSET_LENGTH] -1;
            if(pucDataRead[ucTagCheck] & CRC_MASK == CRC_ERROR_CODE)
            {
                return ERRORCODE_GENERIC;
            }
            if(pucDataRead[2] == 0x88)
                ISO14443A_Card.CascadeLevel =0x02;

            if(pucDataRead[0] == 0x80 && pucDataRead[1] == 0x03)
                return ERRORCODE_GENERIC;
                
            memcpy(ISO14443A_Card.UID , &pucDataRead[2] , 5);
            memcpy(select1+2 ,ISO14443A_Card.UID,5 );   
            
            if(DrvWriteConfig(select1 , sizeof(select1),pucDataRead) == TRUE )
            {
                ISO14443A_Card.SAK = pucDataRead[PCD_DATA_OFFSET];
                
                //apdu_log(DEBUG,"Phone SAK is:%x \r\n" , ISO14443A_Card.SAK);
                
                if(DrvWriteConfig(rats , sizeof(rats),pucDataRead) == TRUE )
                {
                    if(DrvWriteConfig(pps , sizeof(pps),pucDataRead) == TRUE )
                    {
                        detectionPhoneResault = PHONE_DETECTION_SUCCESS;
                        return SEARCH_UUID_SUCCESS;
                    }
                }
            }
        }
    }  
    else
    {
        //������ֻ�Ӳ������ʧ�ܣ�����֮ǰ���ӳɹ����������ʧ�ܴ���
        //��ʧ�ܴ��������涨ֵ�����ʾ�ֻ��Ѿ��뿪
        //����״̬��ΪFAIL����APP������Ϊʧ�ܣ���շ�������������ǰ��2����ʱ������
        //�����ֻ��뿪֮�󻹻����һ�η�����
        if(detectionPhoneResault == PHONE_DETECTION_SUCCESS)
        {
            if(detectionPhoneResaultCnt++ >= DETECTION_LEAVE_MAX)
            {
                apdu_log(DEBUG,"Phone hasbeen leave.\r\n");
                //BeepLedOpen(1 , 0 , 0);
                detectionPhoneResaultCnt = 0;
                readPhoneResault = FALSE;
                detectionPhoneResault = PHONE_DETECTION_FAIL;
            }
        }
    }
    return SEARCH_ERR;
}
uint8_t SelectApplicationFormApdu(uint8_t *ucSelectName)
{
    ApduRecvDataType RecvApdu;
    ApduDataMsgType SelectMsg;
    //uint8_t pucCheckReturnData[]={0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
    uint8_t pucDataRead[256]={0x00};
    //����Select Application����
    if(DrvWriteData(ucSelectName ,ucSelectName[5]+7,pucDataRead) == TRUE)
    {
        memset(&RecvApdu , 0x00 , sizeof (ApduRecvDataType));
        RecvApdu.ucLength = pucDataRead[CR95HF_LENGTH_OFFSET]-ST95_NDEF_DATA_OFFSET_LEN;
        if( RecvApdu.ucLength > 0xF0)
        {
            apdu_log(DEBUG,"Get select application message is to long ,data length = %d .\r\n" , RecvApdu.ucLength);
            goto error;
        }
        memcpy(&RecvApdu ,pucDataRead+ST95_NDEF_DATA_OFFESET , RecvApdu.ucLength);
        
        memset(&SelectMsg , 0x00 , sizeof (ApduDataMsgType));
        SelectMsg.ucLength = RecvApdu.pucMsg[0];
        if( RecvApdu.ucLength > 2)
        {
            if( SelectMsg.ucLength > RecvApdu.ucLength-2)
            {
                PrintApduMessage(pucDataRead);
                apdu_log(DEBUG,"Get select application message is to long ,data length = %d ," , SelectMsg.ucLength);
                apdu_log(DEBUG,"buff length = %d .\r\n" , RecvApdu.ucLength -2 );
                goto error;
            }
            memcpy(SelectMsg.pucMsg ,RecvApdu.pucMsg+1 , RecvApdu.ucLength);
            
            if( ( RecvApdu.ucCmd > APP_TO_DEVICE) || (RecvApdu.ucType != APDU_SELECT_AID))// ||
                //( ByteCompare(SelectMsg.pucMsg , pucCheckReturnData,SelectMsg.ucLength)==0))
            {
                PrintApduMessage(pucDataRead);
                apdu_log(DEBUG,"Select error , cmd = %d ," , RecvApdu.ucCmd);
                apdu_log(DEBUG,"type = %d .\r\n" , RecvApdu.ucType);
                goto error;
            }
            return SELECT_APP_SUCCESS;
        }
        PrintApduMessage(pucDataRead);
    }
error:    
    return SELECT_ERR;
}
/*
1����ѡ��Э��
2��ѡ���ֻ�Ӳ��UUID
3������Select application����
4�����selectʧ�ܣ����ظ����ͣ���Щ�ֻ��������¿������ߣ���������ѡ��UUID֮������ڽ���select�������
5���ɹ����ʾ��app���ӽ����ɹ����������ݽ���
*/
uint8_t ApduSelectTerminusApp(uint8_t *ucReadResault , uint8_t *pucSelectName)
{
    uint8_t ucResult = APDU_INIT;
    uint8_t ucRepeatCnt = 0;
    
    drvCr95hfFiedOn();
    //system_delay(10);
    if( DeviceSelectProtocol() != SELECT_PROTOCOL_SUCCESS)
    {
        goto error ;
    }

    if( SearchPhoneHalUUID() == SEARCH_UUID_SUCCESS)
    {
        ucResult = SelectApplicationFormApdu(pucSelectName);
        while( (ucResult != SELECT_APP_SUCCESS) && (ucRepeatCnt++ < SELECT_REPEAT_MAX) )
        {
            drvCr95hfFiedOff();
            //system_delay(10);
            drvCr95hfFiedOn();
            //system_delay(10);
            if( DeviceSelectProtocol() != SELECT_PROTOCOL_SUCCESS)
            {
                //goto error ;
            }
            if( SearchPhoneHalUUID() == SEARCH_UUID_SUCCESS)
            {
                ucResult = SelectApplicationFormApdu(pucSelectName);
            }
        }
      
        if( ucRepeatCnt >= SELECT_REPEAT_MAX)
        {
            apdu_log(DEBUG,"Select application time out.\r\n");
            goto error;
        }
        else
        {
            apdu_log(DEBUG,"Select applicaiton success. repeat=%d.\r\n" , ucRepeatCnt);  
        }

    }
    else
    {
        goto error;
    }
    *ucReadResault = detectionPhoneResault;  //��ȡ�豸���ֻ�����״̬��������ֻ����ӳɹ����򲻵�ȡNFC�����ӿ�
    return SELECT_APP_SUCCESS;
error:
    *ucReadResault = detectionPhoneResault;
    return SELECT_APP_ERR;
}



uint8_t gucGetUserMessageCnt = 0;



//�����������ֻ�APP�����ҽ��ܷ��ص�����
uint8_t st95WriteMessageToPhone(uint8_t ucCmd,uint8_t ucType,uint8_t *pucSendBUff ,uint8_t ucBuffLength, ApduRecvDataType *pucReadBuff)
{
    ApduType SendApdu;
    uint8_t pucDataRead[256]={0x00};
    uint8_t ucSendLength = 0;

    if(( ucBuffLength  > 0xF0) || (pucSendBUff==NULL))
    {
        apdu_log(DEBUG,"Get send message is to long or send buff is null.\r\n");
        goto error;
    }
    
    if( writeOrReadCmd == CMD_WRITE_MODE)
    {
        writeOrReadCmd = CMD_READ_MODE;
        memcpy(&SendApdu.pHdr , &gucWriteHeard , sizeof(ApduAgreementHeardType));
    }
    else
    {
        writeOrReadCmd = CMD_WRITE_MODE;
        memcpy(&SendApdu.pHdr , &gucReadHeard , sizeof(ApduAgreementHeardType));
    }
    SendApdu.ucCmd = ucCmd;
    SendApdu.ucType = ucType;
    SendApdu.ucDataLength = ucBuffLength;
    memcpy(SendApdu.ucData , pucSendBUff , ucBuffLength);
    SendApdu.ucData[ucBuffLength] = 0x28;
    ucSendLength = ucBuffLength+9;
    
    if(DrvWriteData((uint8_t *)&SendApdu ,ucSendLength,pucDataRead) == TRUE)
    {
        memset(pucReadBuff , 0x00 , sizeof (ApduRecvDataType));
        pucReadBuff->ucLength = pucDataRead[CR95HF_LENGTH_OFFSET]-ST95_NDEF_DATA_OFFSET_LEN;
        if( pucReadBuff->ucLength > 0xF0)
        {
            apdu_log(DEBUG,"Get select application message is to long ,data length = %d .\r\n" , pucReadBuff->ucLength);
            goto error;
        }
        memcpy(pucReadBuff ,pucDataRead+ST95_NDEF_DATA_OFFESET , pucReadBuff->ucLength);
        if(pucReadBuff->ucType == 130 )
        {
            PrintApduMessage(pucDataRead);
        }
        return WRITE_MSG_SUCCESS;
    }
    
    PrintApduMessage(pucDataRead);
error:

    return WRITE_MSG_ERR;
}



