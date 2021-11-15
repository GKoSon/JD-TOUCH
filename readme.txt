##
需求1能刷卡 2能联网
复制ABTOUCH工程 开始工作 名字是TOUCHBLUE+

##
192.168.66.34   3001 此时无法联通
LOG
[#][0%][19-07-22 12:36:26]wifi connect data:AT+CIPSTART=0,"TCP","192.168.66.34",3001

[#][0%][19-07-22 12:36:29]模块返回连接服务器失败 , socket id = 0

此时ONENET地址 很快成功
183.230.40.33  80
LOG
[#][0%][19-07-22 12:42:25]wifi connect data:AT+CIPSTART=0,"TCP","183.230.40.33",80

[#][0%][19-07-22 12:42:25]连接服务器成功 , socket id = 0
[#][0%][19-07-22 12:42:25]MQTT服务器连接成功

#业务小结
上行数据

开门日志
uploadAccessLog
门磁
uploadAccessSensor
设备信息
uploadDeviceInfo
设备事件
uploadDeviceEvent
运维信息
uploadDeviceMaintain
时间同步
timeCalibration
黑白名单请求
filterRequest
消息回复
response
保持联系
keepAlive

修改1 getDeviceId 需要设置的ID

下行
单个下发
dispatchFilterItem 
下发一次性密码
dispatchOTP
控制设备
deviceControlRequest
设备信息 -可能意义不大
uploadDeviceInfoRequest
