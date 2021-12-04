上位机 说明

原料是application.hex ----  它是IAR编译出来的APP的hex文件

双击执行.exe文件----它是win10 cmd编译的 看视频 VScode不能 CMD用g++方可

输出的是application.zip--------它是怎么来的？

1--------1.hex转化为1.bin

2--------然后把1.bin切为N个4K的小bin+最后一个bin可能不是4k

3--------然后对每一个小bin文件 压缩 变成小zip文件

4-------然后把全部小bin文件合并为out.zip

5------然后这个zip的头部需要加入特征的表 类似内存管理表和内存池

前面写的是一个U16--前面1.bin的CRC16

一个U16后面连续有N多少个U16是表示长度的

N个u16每个都是表示一个zip的长度

这个就是最后输出的application.zip



这样单片机去解压的时候

1--U16CRC可以判断证物

2--每个小块读出来解压 最后刚刚好好是4K 正好写一个page

3--当前业务逻辑用版本号做的升级是否成功 比如

正常上网 上报MQTT我的软件版本是1.1.1---MQTT下发升级的url也就是ZIP文件下载路径+文件大小+新版本号---单片机记录上面的信息 其中软件版本号不要更新 也就是一个变量记录1.1.1一个新OTA变量记录2.2.2只有当BOOT程序完成以后才能百分比认为OK了可以更新变量了---单片机下载ZIP文件---验证ZIP的MD5ok---重启--进入BOOT程序---先解压ZIP到SPIFLASH后面空闲地方---对这个解压后的文件做CRC16验证--验证通过--把这个加压文件MV到CHIPFLASH位置---此时把1.1.1更新掉--开始JUMP都APP

也就是 以前是在mqtt_ota.c

            ret = ota_download_read_file();
            if( ret == SOCKET_OK )
            {
                otaType otaCfg;
    
                OTA_DEBUG_LOG(OTA_DEBUG, ("【OTA】文件下载成功\n"));
    
                otaCfg.ver = ota.ver;
                otaCfg.crc32 = ota.crc32;
                otaCfg.fileSize = ota.fileSize;
                otaCfg.otaUpgMark = UPG_MARK;
                config.write(CFG_SYS_SW_VERSION ,&ota.ver , 0);
                config.write(CFG_OTA_CONFIG     ,&otaCfg , TRUE);
                return OTA_OK;
            }

现在需要滞后 在BOOT里面处理

需要删除上面的

config.write(CFG_SYS_SW_VERSION ,&ota.ver , 0);

在BOOT清除flag之前赋值

        if( bootload_download_to_flash() == TRUE)
        {
            cfg.parm.soft_version=cfg.otaVar.ver;
            clear_ota_mark();
            log(INFO,"更新成功\n");
            return TRUE;
        }
