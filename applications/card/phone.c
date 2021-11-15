#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stdlib.h"
#include "sysCfg.h"
#include "unit.h"
#include "phoneTag.h"

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