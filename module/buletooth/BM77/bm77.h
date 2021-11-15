#if (BULETOOTH_MODE == SINGLE_LINK)

#ifndef _BM77_H_
#define _BM77_H_

#include "stdint.h"
#include "unit.h"
#include "buletooth.h"


#define BM77_DELAY  sys_delay

#define  BM77_MAC_CMD		0x0000
#define	 BM77_SLEEP_CMD		0x00EB
#define  BM77_LE_TIME_CMD   0x01C6
#define	 BM77_PWD_CMD		0x0000
#define  BM77_NAME_CMD		0x000B


extern void bm77_receive_timer(void);

extern btDrvType	BM77Drv;
extern void bm77_init(void);

#endif

#endif
