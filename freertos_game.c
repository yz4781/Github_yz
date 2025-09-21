/* USER CODE BEGIN Header */
#include "driver_led.h"
#include "driver_oled.h"
#include "driver_lcd.h"
#include "driver_mpu6050.h"
#include "driver_timer.h"
//#include "NRF24L01.h" 
#include "driver_active_buzzer.h"
#include "driver_passive_buzzer.h"
#include "adc.h" 
#include "driver_ir_receiver.h"
#include "driver_ir_sender.h"
 
#include "driver_ir_obstacle.h"
#include "driver_ultrasonic_sr04.h"
#include "driver_spiflash_w25q64.h"
#include "driver_rotary_encoder.h"
 
 
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
static   xQueueHandle g_xQueueKey1;
static   xQueueHandle g_xQueueKey2;
static   TaskHandle_t xSoundTaskHandle ;
typedef enum {
    KEY_NONE = 0,
    KEY_MENU_LEFT,    
    KEY_MENU_RIGHT,   
    KEY_MENU_BACK,    
    KEY_MENU_CONFIRM, 
 
} KeyEvent;
void game1_task();

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,
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
	 BaseType_t ret;
  LCD_Init();
  LCD_Clear();
// RotaryEncoder_Init();
//  NRF24L01_Init();
//    HAL_ADC_Start_IT(&hadc1);
	
	g_xQueueKey1 = xQueueCreate(10, sizeof(KeyEvent));
	g_xQueueKey2 = xQueueCreate(10, sizeof(KeyEvent));
 
//	    // ��ʾADCֵ�ͶԱȶ�
//    OLED_PrintString(0, 2, "ADC: ");
//     uint16_t adc_val = ADC_GetValue();
//     OLED_PrintSignedVal(4, 2, adc_val);
//    uint8_t contrast = OLED_MapLightToContrast(adc_val);
// 
//    OLED_PrintString(0, 4, "Contrast: ");
//    OLED_PrintSignedVal(9, 4, contrast);
//    
 
    /* USER CODE BEGIN 3 */
//	LCD_PrintString(0, 0, "Starting");
   
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
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
// 
    extern void Music_task();
 

 xTaskCreate(Music_task, "MusicTask", 128, NULL, osPriorityNormal, NULL);
    extern void PlayMusic(void *params);
   xTaskCreate(PlayMusic, "MusicTask", 128, NULL, osPriorityNormal, &xSoundTaskHandle);
  	xTaskCreate(game1_task, "GameTask", 128, NULL, osPriorityNormal, NULL);	

 //    game1_task();
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
	
 //	vTaskDelete(NULL);
	
	LCD_Init();
	LCD_Clear();

	
    IRReceiver_Init();
////	LCD_PrintString(0, 0, "Waiting control");
//    extern void PlayMusic(void *params);
//   xTaskCreate(PlayMusic, "MusicTask", 128, NULL, osPriorityNormal, &xSoundTaskHandle);
	while (1)
	{
//		RotaryEncoder_Test();
// 	    // ��ʾADCֵ�ͶԱȶ�
//    OLED_PrintString(0, 2, "ADC: ");
//     uint16_t adc_val =  HAL_ADC_GetValue(&hadc1);
//     OLED_PrintSignedVal(4, 2, adc_val);
//    uint8_t contrast = OLED_MapLightToContrast(adc_val);
// 
//    OLED_PrintString(0, 4, "Contrast: ");
//    OLED_PrintSignedVal(9, 4, contrast);
//		//    // �Զ���������
		
      OLED_AutoAdjustBrightness();
	}

 
	
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */


