前身是仓库TOUCH-BLUE

2021/11/19 本次简化代码 删除IIC键盘 删除BM77  // 删除临时密码部分

2021/11/20测试刷卡开门//测试白名单功能

注意：需要del的函数 permi_list_init它是自己增加白名单的  

注意：需要del的函数 ble_set_mac它是写MAC是123456的

2021/11/20 准备引入通讯组的修改cfg有问题/同时WIFI不能上网 暂存一次

2021/11/21暂时放弃通讯组修改sys部分 测试ESP32

注意：ESP32上网需要修改如下

`memset(cfg.wifi.ssid , 0x00 , 50);`
`memcpy(cfg.wifi.ssid , "mCube-Xsens-Employee" , strlen("mCube-Xsens-Employee"));`

`memset(cfg.wifi.pwd , 0x00 , 32);`
`memcpy(cfg.wifi.pwd , "Kiss2017" , strlen("Kiss2017"));`
`uint32_t restoreBit = SYS_NET_RESTORE_BIT;`
`config.write(CFG_SET_RESTORE_FLAG , &restoreBit ,TRUE);`

继续MQTT

**MQTT_CLIENT_URL=139.9.66.72:1883**
**MQTT_USER=dark**
**MQTT_PWD=48e8a059e523b9550ac37665ea088cdb**

那么三元组信息还差一个mqttClientId 其实测试已经可以接进去了

`void mqtt_set_default( void )`
`{`
      `memset(&cfg.mqtt, 0x00 ,sizeof(MqttLoginInfoType));`
      `memcpy(cfg.mqtt.mqttUserName , "dark" ,strlen("dark"));`
      `memcpy(cfg.mqtt.mqttUserPwd  , "48e8a059e523b9550ac37665ea088cdb" ,strlen("48e8a059e523b9550ac37665ea088cdb"));`
      `sprintf(cfg.mqtt.mqttClientId,"%032s",(char *)cfg.parm.deviceName);`
`}`

2021/11/21 简化del关于info的部分 只需要cfg文件/ESP32工作还有relay工作必须12V上电

2021/11/22修改MQTT协议代码 此时发现上拉名单会死机 暂存 保存当前问题现场

2021/11/22解决死机问题 订阅的主题是写一个一个数组 发布的主题我是自己放在函数内部的 数组写大一点

2021/11/22测试平台下发时间/同行组/黑白名单 还可以

2021/11/23测试OTA此时代码是主动释放信号量启动OTA任务可以把云平台一个bin文件1234567890下载

*基于当前程序做一个真正的测试bin文件*

*1--修改main里面TESTOTA为1*

*2--编译结果IAR设置输出Seed_STM32L471RE.bin*

*3--本地自己可以测试boot.hex + 0x0800A000 烧写app.bin 就是循环输出*

*4--EWARM\Seed_STM32L471RE\Exe\Seed_STM32L471RE.bin 重命名app.bin放到平台*

![image-20211123160403863](C:\Users\Koson.Gong\AppData\Roaming\Typora\typora-user-images\image-20211123160403863.png)

*准备继续测试*

*1--文件大小是 ota.fileSize=  51484;*

*2--PC点击下载是http://www.ibinhub.com/upload/485982176.bin  memcpy(ota.fileKey,"/upload/485982176.bin" ,strlen("/upload/485982176.bin")  );*

暂存一次 当前可以下载那个简易的bin

2021/11/23测试OTA可以1min之内下载完毕BIN 那个死机是因为log输出大型乱码 需要del 暂存一次 现在准备脚本计算CRC32或者MD5
