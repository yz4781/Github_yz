/* USER CODE BEGIN Header */
#include "driver_led.h"
#include "driver_lcd.h"
#include "driver_mpu6050.h"
#include "driver_timer.h"
#include "driver_active_buzzer.h"
#include "driver_passive_buzzer.h"
#include "driver_ir_receiver.h"
#include "driver_ir_sender.h"
#include "driver_ir_obstacle.h"
#include "driver_ultrasonic_sr04.h"
#include "driver_spiflash_w25q64.h"
#include "driver_rotary_encoder.h"
#include "NRF24L01.h" 
#include "driver_key.h"
#include "driver_uart.h"

/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static StackType_t g_pucStackOfLightTask[128];
static StaticTask_t g_TCBofLightTask;
static TaskHandle_t xLightTaskHandle;

static StackType_t g_pucStackOfColorTask[128];
static StaticTask_t g_TCBofColorTask;
static TaskHandle_t xColorTaskHandle;
void game1_task();
// 按键事件类型枚举
typedef enum {
    KEY_NONE = 0,
    KEY_MENU_LEFT,   // PB1：菜单左选
    KEY_MENU_RIGHT,  // PB2：菜单右选
    KEY_MENU_BACK,   // PB3：菜单返回
    KEY_MENU_CONFIRM,// PB4：菜单确认
    KEY_GAME_LEFT,   // PB5：游戏左移
    KEY_GAME_RIGHT   // PB6：游戏右移
} KeyEvent;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  LCD_Init();
  LCD_Clear();
//  Key_Init ();
  MPU6050_Init();
//  IRReceiver_Init();
//draw_string_P(PSTR("BOTH COLLIDE! RESTART"), false, 10, 30);
//  RotaryEncoder_Init();
	NRF24L01_Init();

//  LCD_PrintString(0, 0, "Starting");

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
//  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
 
//	extern void MPU6050_Task(void *params);
 
	 xTaskCreate(game1_task, "GameTask", 128, NULL, osPriorityNormal, NULL);
// game1_task();

 
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */

/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  uint8_t dev, data;
	int bRunning;
	
	TaskHandle_t xSoundTaskHandle = NULL;
	BaseType_t ret;
	
	vTaskDelete(NULL);
	
	LCD_Init();
	LCD_Clear();

	
    IRReceiver_Init();
	LCD_PrintString(0, 0, "Waiting control");
	
	while (1)
	{
		MPU6050_Test();
	}

    while (1)
    {
        /* 璇诲彇绾㈠閬ユ帶 */
		if (0 == IRReceiver_Read(&dev, &data))
		{		
			if (data == 0xa8) /* play */
			{
				/* 鍒涘缓鎾斁闊充箰鐨勪换鍔? */
			  extern void PlayMusic(void *params);
			  if (xSoundTaskHandle == NULL)
			  {
					LCD_ClearLine(0, 0);
					LCD_PrintString(0, 0, "Create Task");
					ret = xTaskCreate(PlayMusic, "SoundTask", 128, NULL, osPriorityNormal+1, &xSoundTaskHandle);
					bRunning = 1;
			  }
			  else
			  {
				  /* 瑕佷箞suspend瑕佷箞resume */
				  if (bRunning)
				  {
					  LCD_ClearLine(0, 0);
					  LCD_PrintString(0, 0, "Suspend Task");
					  vTaskSuspend(xSoundTaskHandle);
					  PassiveBuzzer_Control(0); /* 鍋滄铚傞福鍣? */
					  bRunning = 0;
				  }
				  else
				  {
					  LCD_ClearLine(0, 0);
					  LCD_PrintString(0, 0, "Resume Task");
					  vTaskResume(xSoundTaskHandle);
					  bRunning = 1;
				  }
			  }
			}
			
			else if (data == 0xa2) /* power */
			{
				/* 鍒犻櫎鎾斁闊充箰鐨勪换鍔? */
				if (xSoundTaskHandle != NULL)
				{
					LCD_ClearLine(0, 0);
					LCD_PrintString(0, 0, "Delete Task");
					vTaskDelete(xSoundTaskHandle);
					PassiveBuzzer_Control(0); /* 鍋滄铚傞福鍣? */
					xSoundTaskHandle = NULL;
				}
			}
		}
    }
	
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

