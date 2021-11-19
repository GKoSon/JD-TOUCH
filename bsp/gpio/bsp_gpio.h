#ifndef _BSP_GPIO_H_
#define _BSP_GPIO_H_


#include <stdio.h>
#include <stdint.h>
#include "stm32l4xx.h"
#include "stm32l4xx_hal.h"

#define GPIO_NULL               (NULL)


#define ST95_IQRI_PIN            (1)            //ST95 irq 
#define RELAY_PIN                (2)            //relay control pin
#define GSM_POWER_PIN            (3)            //gsm resert
#define    ESP12S_RESERT_PIN        (3)            //wifi resert
#define BT_RESERT_PIN           (4)            //bb0906 resert
#define    BM77_RST                (4)            //bm77 resert
#define CONFIG_KEY_PIN          (5)            //config key
#define CY3116_ISR_PIN            (6)            //ct3166 touch key isr
#define DOOR_KEY_PIN              (7)            //open in door pin
#define I2C2_SCL_PIN            (8)
#define I2C2_SDA_PIN            (9)
#define    MAGNET_PIN                (10)        //magnet 
#define NET_CONFIG_PIN1         (11)        //net choose pin1
#define NET_CONFIG_PIN2         (12)         //net choose pin2
#define    RS485_EN_PIN            (13)        //rs485 choose tx or rx
#define    BM77_P20                (14)        //bm77 normal module or hec module
#define W5500_RST_PIN           (15)        //w5500 resert
#define    ST25_IRQ_PIN            (16)        //st25 irq
#define    CY3166_ST95_RST            (16)        //st95 cy3166 resert pin
#define    ST25_CS_PIN                (17)        //st25 cs
#define    CY3166_RST_PIN            (18)        //reservr pin1
#define SYS_STATUS_LED            (19)        //reservr pin1



typedef enum
{
    PIN_LOW,
    PIN_HIGH,
}pinValueEnum;


typedef enum
{
    PIN_MODE_OUTPUT,
    PIN_MODE_PP_PULL,
    PIN_MODE_OUTPUT_PULLUP,
    PIN_MODE_INPUT,
    PIN_MODE_INPUT_PULLUP,
}pinModeEnum;

typedef void    (*exit_callback_fun)( void );


struct _stm32_pin_ops
{
    void (*pin_mode)(uint8_t pin, uint8_t mode);
    void (*pin_exit)(uint8_t pin , exit_callback_fun exit_fun );
    void (*pin_write)(uint8_t pin, uint8_t value);
    int (*pin_read)(uint8_t pin);
    void (*set_mode)(uint8_t pin , uint8_t mode );
};
    
extern const struct _stm32_pin_ops pin_ops;

extern void bsp_gpio_map_init( void );

#endif
