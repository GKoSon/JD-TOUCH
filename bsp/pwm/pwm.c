#include "pwm.h"
#include "timer.h"

#define  PERIOD_VALUE       (uint32_t)(20000 - 1)  /* Period Value  */

#define  PULSE4_VALUE       (uint32_t)(PERIOD_VALUE*200/255) /* Capture Compare 4 Value  */

TIM_OC_InitTypeDef sConfig;
TIM_HandleTypeDef    TimHandle;
__IO uint16_t pwmPeriod = 0;


unsigned char levelPwm[]={1,1,2,2,2,3,3,4,5,6,7,8,9,11,13,16,19,23,27,32,38,45,54,64,76,90,107,128,152,180,214,255,
214,180,152,128,107,90,76,64,54,45,38,32,27,23,19,16,13,11,9,8,7,6,5,4,3,3,2,2,2,1,1};

unsigned char pwmLevel[]={
1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10 ,
11 ,12 ,13 ,14 ,15 ,16 ,17 ,18 ,19 ,20 ,
21 ,22 ,23 ,24 ,25 ,26 ,27 ,28 ,29 ,30 ,
31 ,32 ,33 ,34 ,35 ,36 ,37 ,38 ,39 ,40 ,
41 ,42 ,43 ,44 ,45 ,46 ,47 ,48 ,49 ,50 ,
51 ,52 ,53 ,54 ,55 ,56 ,57 ,58 ,59 ,60 ,
61 ,62 ,63 ,64 ,65 ,66 ,67 ,68 ,69 ,70 ,
71 ,72 ,73 ,74 ,75 ,76 ,77 ,78 ,79 ,80 ,
81 ,82 ,83 ,84 ,85 ,86 ,87 ,88 ,89 ,90 ,
91 ,92 ,93 ,94 ,95 ,96 ,97 ,98 ,99 ,100 ,
101 ,102 ,103 ,104 ,105 ,106 ,107 ,108 ,109 ,110 ,
111 ,112 ,113 ,114 ,115 ,116 ,117 ,118 ,119 ,120 ,
121 ,122 ,123 ,124 ,125 ,126 ,127 ,128 ,129 ,130 ,
131 ,132 ,133 ,134 ,135 ,136 ,137 ,138 ,139 ,140 ,
141 ,142 ,143 ,144 ,145 ,146 ,147 ,148 ,149 ,150 ,
151 ,152 ,153 ,154 ,155 ,156 ,157 ,158 ,159 ,160 ,
161 ,162 ,163 ,164 ,165 ,166 ,167 ,168 ,169 ,170 ,
171 ,172 ,173 ,174 ,175 ,176 ,177 ,178 ,179 ,180 ,
181 ,182 ,183 ,184 ,185 ,186 ,187 ,188 ,189 ,190 ,
191 ,192 ,193 ,194 ,195 ,196 ,197 ,198 ,199 ,200 ,
201 ,202 ,203 ,204 ,205 ,206 ,207 ,208 ,209 ,210 ,
211 ,212 ,213 ,214 ,215 ,216 ,217 ,218 ,219 ,220 ,
221 ,222 ,223 ,224 ,225 ,226 ,227 ,228 ,229 ,230 ,
229 ,229 ,228 ,228 ,227 ,227 ,226 ,226 ,225 ,225 ,
224 ,224 ,223 ,223 ,222 ,222 ,221 ,221 ,220 ,220 ,
219 ,219 ,218 ,218 ,217 ,217 ,216 ,216 ,215 ,215 ,
214 ,214 ,213 ,213 ,212 ,212 ,211 ,211 ,210 ,210 ,
209 ,209 ,208 ,208 ,207 ,207 ,206 ,206 ,205 ,205 ,
204 ,204 ,203 ,203 ,202 ,202 ,201 ,201 ,200 ,200 ,
199 ,199 ,198 ,198 ,197 ,197 ,196 ,196 ,195 ,195 ,
194 ,194 ,193 ,193 ,192 ,192 ,191 ,191 ,190 ,190 ,
189 ,189 ,188 ,188 ,187 ,187 ,186 ,186 ,185 ,185 ,
184 ,184 ,183 ,183 ,182 ,182 ,181 ,181 ,180 ,180 ,
179 ,179 ,178 ,178 ,177 ,177 ,176 ,176 ,175 ,175 ,
174 ,174 ,173 ,173 ,172 ,172 ,171 ,171 ,170 ,170 ,
169 ,169 ,168 ,168 ,167 ,167 ,166 ,166 ,165 ,165 ,
164 ,164 ,163 ,163 ,162 ,162 ,161 ,161 ,160 ,160 ,
159 ,159 ,158 ,158 ,157 ,157 ,156 ,156 ,155 ,155 ,
154 ,154 ,153 ,153 ,152 ,152 ,151 ,151 ,150 ,150 ,
149 ,149 ,148 ,148 ,147 ,147 ,146 ,146 ,145 ,145 ,
144 ,144 ,143 ,143 ,142 ,142 ,141 ,141 ,140 ,140 ,
139 ,139 ,138 ,138 ,137 ,137 ,136 ,136 ,135 ,135 ,
134 ,134 ,133 ,133 ,132 ,132 ,131 ,131 ,130 ,130 ,
129 ,129 ,128 ,128 ,127 ,127 ,126 ,126 ,125 ,125 ,
124 ,124 ,123 ,123 ,122 ,122 ,121 ,121 ,120 ,120 ,
119 ,119 ,118 ,118 ,117 ,117 ,116 ,116 ,115 ,115 ,
114 ,114 ,113 ,113 ,112 ,112 ,111 ,111 ,110 ,110 ,
109 ,109 ,108 ,108 ,107 ,107 ,106 ,106 ,105 ,105 ,
104 ,104 ,103 ,103 ,102 ,102 ,101 ,101 ,100 ,100 ,
99 ,99 ,98 ,98 ,97 ,97 ,96 ,96 ,95 ,95 ,
94 ,94 ,93 ,93 ,92 ,92 ,91 ,91 ,90 ,90 ,
89 ,89 ,88 ,88 ,87 ,87 ,86 ,86 ,85 ,85 ,
84 ,84 ,83 ,83 ,82 ,82 ,81 ,81 ,80 ,80 ,
79 ,79 ,78 ,78 ,77 ,77 ,76 ,76 ,75 ,75 ,
74 ,74 ,73 ,73 ,72 ,72 ,71 ,71 ,70 ,70 ,
69 ,69 ,68 ,68 ,67 ,67 ,66 ,66 ,65 ,65 ,
64 ,64 ,63 ,63 ,62 ,62 ,61 ,61 ,60 ,60 ,
59 ,59 ,58 ,58 ,57 ,57 ,56 ,56 ,55 ,55 ,
54 ,54 ,53 ,53 ,52 ,52 ,51 ,51 ,50 ,50 ,
49 ,49 ,48 ,48 ,47 ,47 ,46 ,46 ,45 ,45 ,
44 ,44 ,43 ,43 ,42 ,42 ,41 ,41 ,40 ,40 ,
39 ,39 ,38 ,38 ,37 ,37 ,36 ,36 ,35 ,35 ,
34 ,34 ,33 ,33 ,32 ,32 ,31 ,31 ,30 ,30 ,
29 ,29 ,28 ,28 ,27 ,27 ,26 ,26 ,25 ,25 ,
24 ,24 ,23 ,23 ,22 ,22 ,21 ,21 ,20 ,20 ,
19 ,19 ,18 ,18 ,17 ,17 ,16 ,16 ,15 ,15 ,
14 ,14 ,13 ,13 ,12 ,12 ,11 ,11 ,10 ,10 ,
9 ,9 ,8 ,8 ,7 ,7 ,6 ,6 ,5 ,
5 ,4 ,4 ,3 ,3 ,2 ,2 ,1 ,1 ,
};

uint8_t addorless = false;

void pwm_isr( void )
{
    HAL_TIM_PWM_Stop(&TimHandle, TIM_CHANNEL_4);
    
    sConfig.Pulse        = (uint32_t)(PERIOD_VALUE*pwmLevel[pwmPeriod]/255) ;
    //sConfig.Pulse        = (uint32_t)(PERIOD_VALUE*pwmPeriod/255) ;
    HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_4);

    if( ++pwmPeriod == sizeof(pwmLevel)-1 )
    {
        pwmPeriod = 0;
    }
    
    HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_4);
}

void tim_pwm_mspinit( void )
{
    GPIO_InitTypeDef   GPIO_InitStruct;

    /* TIMx Peripheral clock enable */
    __HAL_RCC_TIM2_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}


void pwm_init(void)
{
    

    /* Timer Output Compare Configuration Structure declaration */
    
    
    uint32_t uhPrescalerValue = 0;
    
    uhPrescalerValue = (uint32_t)(SystemCoreClock / 16000000) - 1;

    tim_pwm_mspinit();
    
    TimHandle.Instance = TIM2;
    TimHandle.Init.Prescaler         = uhPrescalerValue;
    TimHandle.Init.Period            = PERIOD_VALUE;
    TimHandle.Init.ClockDivision     = 0;
    TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    TimHandle.Init.RepetitionCounter = 0;
    if (HAL_TIM_PWM_Init(&TimHandle) != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.OCMode       = TIM_OCMODE_PWM1;
    sConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfig.OCFastMode   = TIM_OCFAST_DISABLE;
    sConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    sConfig.OCIdleState  = TIM_OCIDLESTATE_RESET;
    sConfig.Pulse        = PULSE4_VALUE;
    
    if (HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }
    
    
    timer.creat(15,TRUE,pwm_isr);
}
