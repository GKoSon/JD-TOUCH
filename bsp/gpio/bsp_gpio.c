/*
 * File      : gpio.c
 */

#include "bsp_gpio.h"
#include "unit.h"


/* STM32 GPIO driver */
struct pin_index
{
    int             index;
    GPIO_TypeDef    *gpio;
    uint16_t        pin;
    uint32_t        mode;
    uint32_t        pull;
};


struct pin_exit
{
    uint32_t                pin;
    IRQn_Type               irq;
    uint8_t                 priority;
    exit_callback_fun       fun;
};

static  struct pin_exit exits[]=
{
    {GPIO_PIN_0 ,  EXTI0_IRQn ,   13 , NULL },
    {GPIO_PIN_1 ,  EXTI1_IRQn ,   13 , NULL },
    {GPIO_PIN_2 ,  EXTI2_IRQn ,   13 , NULL },
    {GPIO_PIN_3 ,  EXTI3_IRQn ,   13 , NULL },
    {GPIO_PIN_4 ,  EXTI4_IRQn ,   13 , NULL },
    {GPIO_PIN_5 ,  EXTI9_5_IRQn,  13 , NULL },
    {GPIO_PIN_6 ,  EXTI9_5_IRQn,  13 , NULL },
    {GPIO_PIN_7 ,  EXTI9_5_IRQn,  13 , NULL },
    {GPIO_PIN_8 ,  EXTI9_5_IRQn,  13 , NULL },
    {GPIO_PIN_9 ,  EXTI9_5_IRQn,  13 , NULL },
    {GPIO_PIN_10 , EXTI15_10_IRQn,13 , NULL },
    {GPIO_PIN_11 , EXTI15_10_IRQn,13 , NULL },
    {GPIO_PIN_12 , EXTI15_10_IRQn,13 , NULL },
    {GPIO_PIN_13 , EXTI15_10_IRQn,13 , NULL },
    {GPIO_PIN_14 , EXTI15_10_IRQn,13 , NULL },
    {GPIO_PIN_15 , EXTI15_10_IRQn,13 , NULL },
    
};



static const struct pin_index pins[] =
{
    { 0 , GPIOC , GPIO_PIN_8    , GPIO_MODE_OUTPUT_PP    , GPIO_NOPULL},
    { 1 , GPIOC , GPIO_PIN_6    , GPIO_MODE_OUTPUT_OD    , GPIO_PULLUP},
    { 2 , GPIOA , GPIO_PIN_12    , GPIO_MODE_OUTPUT_PP    , GPIO_NOPULL},
    { 3 , GPIOB , GPIO_PIN_2    , GPIO_MODE_OUTPUT_PP    , GPIO_PULLUP},
    { 4 , GPIOB , GPIO_PIN_0    , GPIO_MODE_OUTPUT_PP    , GPIO_PULLUP},
    { 5 , GPIOC , GPIO_PIN_9    , GPIO_MODE_IT_FALLING    , GPIO_PULLUP},
    { 6 , GPIOB , GPIO_PIN_7    , GPIO_MODE_IT_FALLING    , GPIO_PULLUP},
    { 7 , GPIOA , GPIO_PIN_11    , GPIO_MODE_IT_FALLING    , GPIO_PULLUP},
    { 8 , GPIOB , GPIO_PIN_8     , GPIO_MODE_OUTPUT_PP , GPIO_PULLUP},
    { 9 , GPIOB , GPIO_PIN_9     , GPIO_MODE_OUTPUT_PP , GPIO_PULLUP},
    { 10 , GPIOA , GPIO_PIN_8    , GPIO_MODE_INPUT        , GPIO_NOPULL},
    { 11 , GPIOC , GPIO_PIN_13    , GPIO_MODE_INPUT        , GPIO_NOPULL},
    { 12 , GPIOC , GPIO_PIN_3    , GPIO_MODE_INPUT        , GPIO_NOPULL},
    { 13 , GPIOC , GPIO_PIN_11    , GPIO_MODE_OUTPUT_PP    , GPIO_NOPULL},
    { 14 , GPIOC , GPIO_PIN_2    , GPIO_MODE_OUTPUT_PP    , GPIO_NOPULL},
    { 15 , GPIOC , GPIO_PIN_10    , GPIO_MODE_OUTPUT_PP    , GPIO_NOPULL},
    { 16 , GPIOB , GPIO_PIN_1    , GPIO_MODE_IT_RISING    , GPIO_NOPULL},
    { 17 , GPIOB , GPIO_PIN_12    , GPIO_MODE_OUTPUT_PP    , GPIO_NOPULL},
    { 18 , GPIOC , GPIO_PIN_7    , GPIO_MODE_OUTPUT_PP    , GPIO_NOPULL},
    { 19 , GPIOB , GPIO_PIN_10    , GPIO_MODE_OUTPUT_PP        , GPIO_NOPULL},
};


#define ITEM_NUM(items) sizeof(items)/sizeof(items[0])

const struct pin_index *get_pin(uint8_t pin)
{

    for(uint8_t i = 0 ; i < ITEM_NUM(pins) ; i++)
    {
        if( pin == pins[i].index)
        {
            return &pins[i];
        }
    }
         
    return NULL;
};

void stm32_pin_write(uint8_t pin, uint8_t value)
{
    const struct pin_index *index;

    index = get_pin(pin);
    if (index == GPIO_NULL)
    {        
        return;
    }

    if (value == PIN_LOW)
    {
        HAL_GPIO_WritePin(index->gpio, index->pin , GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(index->gpio, index->pin , GPIO_PIN_SET);
    }
}

int stm32_pin_read(uint8_t pin)
{
    int value;
    const struct pin_index *index;

    value = PIN_LOW;

    index = get_pin(pin);
    if (index == GPIO_NULL)
    {
        return value;
    }

    if (HAL_GPIO_ReadPin(index->gpio, index->pin) == GPIO_PIN_RESET)
    {
        value = PIN_LOW;
    }
    else
    {
        value = PIN_HIGH;
    }

    return value;
}

void stm32_pin_mode(uint8_t pin, uint8_t mode)
{
    const struct pin_index *index;
    GPIO_InitTypeDef GPIO_InitStruct;
    
    
    index = get_pin(pin);
    if (index == GPIO_NULL)
    {
        log(WARN,"映射序号%d , 没有对应的映射表\n" , pin);
        return;
    }
    
    HAL_GPIO_WritePin(index->gpio, index->pin, GPIO_PIN_RESET);
    
    GPIO_InitStruct.Mode = index->mode;
    GPIO_InitStruct.Pull = index->pull;    
    GPIO_InitStruct.Pin = index->pin;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(index->gpio, &GPIO_InitStruct);
       
}

struct pin_exit *get_exit(uint16_t pin)
{
    for(uint8_t i = 0 ; i < ITEM_NUM(exits) ; i++)
    {
        if(pin == exits[i].pin)
        {
            return &exits[i];
        }
    }
    
    return NULL;
}


void stm32_pin_exit(uint8_t pin , exit_callback_fun exit_fun )
{    
    
    struct pin_exit *exit;
    const struct pin_index *index;
    
    index = get_pin(pin);
    if (index == GPIO_NULL)
    {
        return;
    }
    
    exit = get_exit(index->pin);
    exit->fun = exit_fun;
    
    HAL_NVIC_SetPriority(exit->irq, exit->priority, 0);
    HAL_NVIC_EnableIRQ(exit->irq);
    
}

void stm32_pin_set_mode(uint8_t pin , uint8_t mode )
{
    const struct pin_index *index;
    GPIO_InitTypeDef GPIO_InitStruct;
    
    
    index = get_pin(pin);
    if (index == GPIO_NULL)
    {
        return;
    }
    
    if( mode == PIN_MODE_INPUT_PULLUP)
    {
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
    }
    else if( mode == PIN_MODE_INPUT)
    {
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }
    else if(  mode == PIN_MODE_OUTPUT_PULLUP)
    {
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
    }
    else if(  mode == PIN_MODE_OUTPUT)
    {
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }
    else 
    {
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
    }     
    GPIO_InitStruct.Pin = index->pin;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(index->gpio, &GPIO_InitStruct);
}
const struct _stm32_pin_ops pin_ops =
{
    stm32_pin_mode,
    stm32_pin_exit,
    stm32_pin_write,
    stm32_pin_read,
    stm32_pin_set_mode,
};

void EXTI0_IRQHandler(void)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
        if(exits[0].fun != NULL)
        {
            exits[0].fun();
        }
    }    
}
void EXTI1_IRQHandler(void)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_1) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);
        if(exits[1].fun != NULL)
        {
            exits[1].fun();
        }
    } 
}
void EXTI2_IRQHandler(void)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_2) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);
        if(exits[2].fun != NULL)
        {
            exits[2].fun();
        }
    } 
}
void EXTI3_IRQHandler(void)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_3) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
        if(exits[3].fun != NULL)
        {
            exits[3].fun();
        }
    } 
}
void EXTI4_IRQHandler(void)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_4) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4);
        if(exits[4].fun != NULL)
        {
            exits[4].fun();
        }
    } 
}
void EXTI9_5_IRQHandler(void)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5);
        if(exits[5].fun != NULL)
        {
            exits[5].fun();
        }
    } 
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_6) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);
        if(exits[6].fun != NULL)
        {
            exits[6].fun();
        }
    } 
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_7) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);
        if(exits[7].fun != NULL)
        {
            exits[7].fun();
        }
    } 
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_8) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_8);
        if(exits[8].fun != NULL)
        {
            exits[8].fun();
        }
    } 
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_9) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);
        if(exits[9].fun != NULL)
        {
            exits[9].fun();
        }
    } 
}

void EXTI15_10_IRQHandler(void)
{

    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_10) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_10);
        if(exits[10].fun != NULL)
        {
            exits[10].fun();
        }
    } 
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);
        if(exits[11].fun != NULL)
        {
            exits[11].fun();
        }
    } 
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_12) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_12);
        if(exits[12].fun != NULL)
        {
            exits[12].fun();
        }
    } 
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
        if(exits[13].fun != NULL)
        {
            exits[13].fun();
        }
    } 
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_14) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_14);
        if(exits[14].fun != NULL)
        {
            exits[14].fun();
        }
    } 
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_15);
        if(exits[15].fun != NULL)
        {
            exits[15].fun();
        }
    } 
    
}




void bsp_gpio_map_init( void )
{
    
}