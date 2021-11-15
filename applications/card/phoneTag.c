#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stdlib.h"
#include "sysCfg.h"
#include "unit.h"
#include "phoneTag.h"
#include "swipeTag.h"
#include "tagComponent.h"

uint8_t gucWriteOrReadCmd = CMD_WRITE_MODE;                //�������ͣ�ģʽ�л�
uint8_t gucDetectionPhoneResault = PHONE_DETECTION_FAIL;   //�ж��Ƿ��⵽�ֻ�
uint8_t gucDetectionPhoneResaultCnt = 0;                   //������
uint8_t gucReadPhoneResault = FALSE;                       //�ж��Ƿ���APPͨѶ�ɹ�
uint32_t gdOpenTimeFormPhone = 0;      //����ʱ��    

uint8_t pucReturnApduData[6]={0x0A,0x0A,0x0A,0x0A,0x0A,0x0A};  //Э������ֶ�


/***
**һ��һ��ģʽ������������Ҫ�ȷ���select cc file�� �ڷ���read
**��������������Ҫÿ�ζ��л�Э��ͷ
***/
ApduAgreementHeardType gucWriteHeard =
{
    .DrvCommand = 0x03, 
    //apdu
    .Cla = 0x00,        
    .Ins = 0xA4,
    .P1 = 0x00,
    .P2 = 0x00,
};

ApduAgreementHeardType gucReadHeard =
{
    .DrvCommand = 0x02,
    .Cla = 0x00,
    .Ins = 0xB0,
    .P1 = 0x00,
    .P2 = 0x00,

};



uint8_t encrypt(uint8_t *data, uint8_t cipherKey , uint8_t length) 
{
    for (int i = 0; i < length; i++) 
    {
        data[i] = ((uint8_t) ((data[i] ^ cipherKey) & 0xff));
    }
    return TRUE;
}

uint8_t decrypt(uint8_t *data, uint8_t cipherKey , uint8_t length) 
{
    for (int i = 0; i < length; i++) 
    {
        data[i] = ((uint8_t) ((data[i] ^ cipherKey) & 0xff));
    }
    return TRUE;
}

uint8_t encryptCommand(uint8_t *command,uint8_t size, uint8_t *keyWord , uint8_t length) 
{

    for (int i = 0; i < length; i++) 
    {
        encrypt(command, keyWord[i] , size);
    }
    
    return TRUE;
}

uint8_t decryptCommand(uint8_t *command,uint8_t size, uint8_t *keyWord , uint8_t length)
{
    while(length--)
    {
        decrypt(command, keyWord[length],size);
    }
    return TRUE;
}

uint8_t mystrcmp(uint8_t *ps , uint8_t *str , uint8_t len)
{
    uint8_t i =0;
    
    if(( ps == NULL) || (str ==NULL))
    {
        return FALSE;
    }
    for( i = 0 ; i < len ; i++)
    {
        if( ps[i] != str[i])
        {
            return FALSE;
        }
    }
    return TRUE;
}

uint8_t  DertyptCheckPassword( ApduDataMsgType *pucApdu ,uint8_t *pucPwd)
{

    uint8_t password[4]={0x00};
    uint8_t DertyptMsg[256];
    uint8_t rt = FALSE;
    uint8_t i =0;

    if( pucPwd == NULL)
    {
        return FALSE;
    }
    
    memset(DertyptMsg ,0x00,256);
    
    if( pucApdu->ucLength > 0xF0 )
    {
        log(DEBUG,"Recv password is to long , length = %d.\r\n" ,pucApdu->ucLength);
        return rt;
    }
    memcpy(DertyptMsg ,pucApdu->pucMsg ,pucApdu->ucLength );
    
    
    for(i = 0; i < 3; i++)
    {
        password[i] = pucPwd[i];//gFKInfo.sysType.sysData.uSecret[i];
    }    

    decryptCommand(DertyptMsg , pucApdu->ucLength, password , 3 );

    rt= mystrcmp(OPEN_DOOR_CMD, DertyptMsg , strlen(OPEN_DOOR_CMD));   
    
    return rt;
}



uint8_t CkeckReturnMessage(ApduRecvDataType *ApduMsg , uint8_t GetCMD)
{
    ApduDataMsgType SelectMsg;
    //unsigned char pucCheckReturnData[]={0x0C,0x0C,0x0C,0x0C,0x0C,0x0C};
    
    if( ( ApduMsg->ucCmd < APP_TO_DEVICE) && (ApduMsg->ucType == GetCMD) )
    {
        memset(&SelectMsg , 0x00 , sizeof (ApduDataMsgType));
        SelectMsg.ucLength = ApduMsg->pucMsg[0];
        if( SelectMsg.ucLength > ApduMsg->ucLength-2)
        {
            log(DEBUG,"Get return message is to long ,data length = %d ," , SelectMsg.ucLength);
            log(DEBUG,"buff length = %d .\r\n" , ApduMsg->ucLength-2);
            return FALSE;
        }
        memcpy(SelectMsg.pucMsg ,ApduMsg->pucMsg+1 , ApduMsg->ucLength);
        //if( ByteCompare(SelectMsg.pucMsg , pucCheckReturnData,SelectMsg.ucLength)==1)
        {
            return TRUE;
        }
        
    }
    return FALSE;
}


//extern void subOpenJdq(uint8_t ic_door);
//���������
uint8_t OpenDoorUseNFCForPhone(ApduRecvDataType *ApduMsg)
{
    ApduDataMsgType SelectMsg;
    uint8_t ucReturn = APDU_INIT;
    uint8_t *pairPwd, *userPwd;
    
    if( ( ApduMsg->ucCmd < APP_TO_DEVICE) && (ApduMsg->ucType == APDU_OPEN_COMMAND) )
    {
        memset(&SelectMsg , 0x00 , sizeof (ApduDataMsgType));
        SelectMsg.ucLength = ApduMsg->pucMsg[0];
        if( SelectMsg.ucLength > ApduMsg->ucLength-2)
        {
            log(DEBUG,"Get open door message is to long ,data length = %d ," , SelectMsg.ucLength);
            log(DEBUG,"buff length = %d .\r\n" , ApduMsg->ucLength-2);
            ucReturn = OPEN_DOOR_ERR;
        }
        memcpy(SelectMsg.pucMsg ,ApduMsg->pucMsg+1 , ApduMsg->ucLength);
        
        
        config.read(CFG_PAIR_PWD,(void **)&pairPwd);
        config.read(CFG_USER_PWD,(void **)&userPwd);
        
		if( DertyptCheckPassword( &SelectMsg, pairPwd) != TRUE )
		{
			if( DertyptCheckPassword( &SelectMsg, userPwd) != TRUE )
			{
				return OPEN_PASSWORD_ERR;
			}
		}
		gucReadPhoneResault = TRUE; //���ųɹ���־��λ
		ucReturn = OPEN_SUCCESS;
	}
	
    return ucReturn;
}


uint8_t tag_read_phone_nfc( void )
{
    ApduRecvDataType ApduUserMsg;
    uint8_t ApduRunStatue = SEND_MAC;
    uint8_t *pucDeviceMac = NULL;    //�豸������ַ
    uint8_t ucReturn = TAG_PHONE_ERR;
    
	gucWriteOrReadCmd = CMD_WRITE_MODE;
    config.read(CFG_BLE_MAC , (void **)&pucDeviceMac);
    
agin:
    switch(ApduRunStatue)
    {
        case SEND_MAC:
        {
            //�����豸����MAC��ַ
            if( tagComp->write_and_read_app(APDU_NFC_OPEN,APDU_SEND_MAC,pucDeviceMac,DEVICE_MAC_SIZE , &ApduUserMsg)!= WRITE_MSG_SUCCESS)
            {
                goto end;
            }
        }break;
        case SNED_SEARCH_KEY:
        {
            //��������Կ��
            if( tagComp->write_and_read_app(APDU_NFC_OPEN,APDU_SEARCH_KEY,pucReturnApduData,RETURN_DATA_SIZE , &ApduUserMsg) != WRITE_MSG_SUCCESS)
            {
                goto end;
            }
        }break;
        case SEND_OPEN_SUCCESS:
        {
            //���Ϳ��ųɹ�
            if( tagComp->write_and_read_app(APDU_NFC_OPEN,APDU_OPEN_SUCCESS,pucReturnApduData,RETURN_DATA_SIZE , &ApduUserMsg) != WRITE_MSG_SUCCESS)
            {
                goto end;
            }
        }break;
        case SEND_OPEN_FAIL:
        {
            //���Ϳ���ʧ��
            if( tagComp->write_and_read_app(APDU_NFC_OPEN,APDU_OPEN_FAIL,pucReturnApduData,RETURN_DATA_SIZE , &ApduUserMsg) != WRITE_MSG_SUCCESS)
            {
                goto end;
            }
        }break;   
        case SEND_PASSWORD_ERR:
        {
            //���Ϳ���ʧ��
            if( tagComp->write_and_read_app(APDU_NFC_OPEN,APDU_PASSWORD_ERR,ApduUserMsg.pucMsg+1,ApduUserMsg.ucLength-3 , &ApduUserMsg) != WRITE_MSG_SUCCESS)
            {
                goto end;
            }
        }break;
        default:log(DEBUG,"recv a error send com = %d.\r\n" ,ApduRunStatue ); goto end;
    }
      
    switch(ApduUserMsg.ucType)
    {
            //App׼������Կ�ף������ڷ�������Կ������
        case APDU_READY_HANDLE:
        {
            log(DEBUG,"Accept ready command.\r\n");
            if( CkeckReturnMessage(&ApduUserMsg , APDU_READY_HANDLE) == TRUE)
            {
                ApduRunStatue = SNED_SEARCH_KEY;
                goto agin;  
            }
        }break;
        //����ָ��,������󷵻ؿ���ʧ�ܣ�������ȷ���ؿ��ųɹ�
        //�����ʾ���Ŵ����˳�����ͨѶ
        case APDU_OPEN_COMMAND:
        {
            log(DEBUG,"Accept open door command.\r\n");
            switch( OpenDoorUseNFCForPhone(&ApduUserMsg) )
            {
                case OPEN_PASSWORD_ERR: 
                {
                    uint8_t i =0;
                    ApduRunStatue = SEND_PASSWORD_ERR ;
                    log(DEBUG,"Open door fail , password is error.\r\n");
                    log(DEBUG,"Password is =");
                    for(i = 0 ; i < ApduUserMsg.ucLength ; i++)
                    {
                        log(DEBUG,"0x%X " , ApduUserMsg.pucMsg[i]);
                    }
                    log(DEBUG,"\r\n");
                    
                    ucReturn = TAG_PHONE_PWD_ERR;
                    
                    goto agin;
                }
                case OPEN_SUCCESS:
                {
                    ApduRunStatue = SEND_OPEN_SUCCESS;
                    log(DEBUG,"Open door success.\r\n");
                    ucReturn = TAG_SUCESS;
                    goto agin;
                }
                case OPEN_DOOR_ERR:
                default:
                  
                    ucReturn = TAG_PHONE_ERR;
    
            }break;

        }break;
        case APDU_WAIT_HANDLE:
        {
            if( CkeckReturnMessage(&ApduUserMsg , APDU_WAIT_HANDLE) == TRUE)
            {
                log(DEBUG,"Accept wait command success.\r\n");
                ApduRunStatue = SNED_SEARCH_KEY;
                goto agin;  
            }
        }break;
            //App����û��Կ�ף��������������������ҷ��ؿ���ʧ��
        case APDU_NO_KEY:
        {     
            log(DEBUG,"Accept no key command.\r\n"); 
            if( CkeckReturnMessage(&ApduUserMsg , APDU_NO_KEY) == TRUE)
            {
                ApduRunStatue = SEND_OPEN_FAIL;
                ucReturn = TAG_PHONE_NO_KEY_ERR;
                goto agin;  
            }
        }break;
            //App�����޷�ʶ����ܵ���������ؿ���ʧ��
        case APDU_UNKNOW_COMMAND:
        {
            if( CkeckReturnMessage(&ApduUserMsg , APDU_UNKNOW_COMMAND) == TRUE)
            {
                ApduRunStatue = SEND_OPEN_FAIL;
                ucReturn = TAG_PHONE_ERR;
                goto agin;  
            }
        }break;
            //�رմ˴�ͨѶ
        case APDU_CLOSE_COMMADN:
        {
            log(DEBUG,"Accept close fied command.\r\n");
            if( CkeckReturnMessage(&ApduUserMsg , APDU_CLOSE_COMMADN) == TRUE)
            {
                log(DEBUG,"operation time = %dms.\r\n" , gdOpenTimeFormPhone);
            }
        }break;
            //���·���MAC
        case APDU_REPEAT_MAC:
        {
            log(DEBUG,"Accept repeat mac command.\r\n");
            if( CkeckReturnMessage(&ApduUserMsg , APDU_REPEAT_MAC) == TRUE)
            {
                ApduRunStatue = SEND_MAC;
                goto agin; 
            }
        }break;
        default : 
                ucReturn = TAG_PHONE_ERR;
                log(DEBUG,"Accept type = %d .\r\n" , ApduUserMsg.ucType); 
                break;;          
    }            
end:

    return ucReturn;
}

