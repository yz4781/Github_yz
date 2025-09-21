#include "driver_rotary_encoder.h"
#include "driver_lcd.h"
#include "driver_timer.h"
#include "stm32f1xx_hal.h"
#include "tim.h"


 

int16_t Encoder_Count = 0;  // 编码器计数变量

void Encoder_Init(void)
{
    // CubeMX已配置好GPIO和中断，这里可放置额外初始化代码
    // 使能中断
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

int16_t Encoder_Get(void)
{
    int16_t Temp;
    Temp = Encoder_Count;
    Encoder_Count = 0;  // 读取后清零，用于累计外部变量
//		if (Temp >=50 )Temp  =50;
//	else if(Temp  <=-35)Temp  =-35;
    return Temp;
}

// PB0中断回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        // 检测PB1电平判断转向
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET)
        {
            Encoder_Count--;  // 反转
        }
    }
    else if (GPIO_Pin == GPIO_PIN_1)
    {
        // 检测PB0电平判断转向
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET)
        {
            Encoder_Count++;  // 正转
        }
    }
}

