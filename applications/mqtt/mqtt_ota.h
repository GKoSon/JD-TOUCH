#ifndef _MQTT_OTA_H_
#define _MQTT_OTA_H_

#define     USER_FLASH_END_ADDRESS             0x0807A000

#define        UPG_MARK                          0xAABB55AA
#define        OTA_OK                            SOCKET_OK

typedef enum
{
    CHECH_UPG_FILE,
    DOWMLOAD_FILE,
    SYS_UPG_MODE,
}otaStatusEnum;


typedef enum
{
    OTA_UPG_INIT = -60,
    OTA_UPG_CONNECTERR,
    OTA_UPG_SENDERR,
    OTA_UPG_PUCKERR,
    OTA_UPG_READERR,
    OTA_UPG_DATA_ERR,
    OTA_UPG_ANALYSISERR,
    OTA_UPG_NOVER,
    OTA_DOWNLOAD_FAIL,
}otaErrEnum;


typedef struct
{
    otaStatusEnum    otaStatus;

    uint32_t        len;//�Լ������
    uint32_t        ver;//�汾��
    uint32_t        fileSize;//�ļ�����
    uint32_t        crc32;//MD5�ļ�
    char           fileKey[50];//���ص�URL
}otaRecvCmdType;

extern otaRecvCmdType  ota;

void creat_mqtt_ota_task( void );
void ota_start_upg(void);
#endif
