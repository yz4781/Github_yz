#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <main.h>
 
#include "cmsis_os.h"
#include "FreeRTOS.h"                   // ARM.FreeRTOS::RTOS:Core
#include "task.h"                       // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h"               // ARM.FreeRTOS::RTOS:Event Groups
#include "semphr.h"                     // ARM.FreeRTOS::RTOS:Core
#include "NRF24L01.h"  
#include "draw.h"
#include "resources.h"
#include "driver_key.h"
#include "driver_lcd.h"
#include "driver_ir_receiver.h"
#include "driver_rotary_encoder.h"
#include "driver_mpu6050.h"
#include "driver_uart.h" 
#define NOINVERT	false
#define INVERT		true

#define sprintf_P  sprintf
#define PSTR(a)  a

#define PLATFORM_WIDTH	12
#define PLATFORM_HEIGHT	4
#define UPT_MOVE_NONE	0
#define UPT_MOVE_RIGHT	1
#define UPT_MOVE_LEFT	2
#define BLOCK_COLS		32
#define BLOCK_ROWS		5
#define BLOCK_COUNT		(BLOCK_COLS * BLOCK_ROWS)


 // 数据类型标识（区分设备数据和Key值）
#define DATA_TYPE_DEVICE 0x00  // 设备数据标识
#define DATA_TYPE_KEY 0x01     // Key值标识
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
typedef struct{
	float x;
	float y;
	float velX;
	float velY;
}s_ball;

static const byte block[] ={
	0x07,0x07,0x07,
};

static const byte platform[] ={
	0x60,0x70,0x50,0x10,0x30,0xF0,0xF0,0x30,0x10,0x50,0x70,0x60,
};

static const byte ballImg[] ={
	0x03,0x03,
};

static const byte clearImg[] ={
	0,0,0,0,0,0,0,0,0,0,0,0,
};

static bool btnExit(void);
static bool btnRight(void);
static bool btnLeft(void);
void game1_draw(void);

static byte uptMove;
static s_ball ball;
static bool* blocks;
static byte lives, lives_origin;
static uint score;
static byte platformX;

static uint32_t g_xres, g_yres, g_bpp;
static uint8_t *g_framebuffer;
static QueueSetHandle_t g_xQueueSetInput; /* 输入设备的队列集 */
static QueueHandle_t g_xQueuePlatform; /* 挡球板队列 */
static QueueHandle_t g_xQueueIR;
static QueueHandle_t g_xQueuekey_game;
static QueueHandle_t g_xQueueRotary;
static QueueHandle_t g_xQueueMPU6050; /* MPU6050队列 */
static QueueHandle_t g_xQueueKey; /* KEY队列 */
 
 

static uint8_t NRF24L01_TxPacket[2] = {0};
static uint8_t NRF24L01_RxPacket[2] = {0};

 

/* 挡球板任务 */
static void platform_task(void *params)
{
  byte platformXtmp = platformX;    
	struct input_data idata;

	// Draw platform
	draw_bitmap(platformXtmp, g_yres - 8, platform, 12, 8, NOINVERT, 0);
	draw_flushArea(platformXtmp, g_yres - 8, 12, 8);
	
	while (1)
	{
		//if (0 == IRReceiver_Read(&dev, &data))
		xQueueReceive(g_xQueuePlatform, &idata, portMAX_DELAY);
		
		uptMove = idata.val;
						
		// Hide platform
		draw_bitmap(platformXtmp, g_yres - 8, clearImg, 12, 8, NOINVERT, 0);
		draw_flushArea(platformXtmp, g_yres - 8, 12, 8);
		
		// Move platform
		if(uptMove == UPT_MOVE_RIGHT)
			platformXtmp += 3;
		else if(uptMove == UPT_MOVE_LEFT)
			platformXtmp -= 3;
		uptMove = UPT_MOVE_NONE;
		
		// Make sure platform stays on screen
		if(platformXtmp > 250)
			platformXtmp = 0;
		else if(platformXtmp > g_xres - PLATFORM_WIDTH)
			platformXtmp = g_xres - PLATFORM_WIDTH;
		
		// Draw platform
		draw_bitmap(platformXtmp, g_yres - 8, platform, 12, 8, NOINVERT, 0);
		draw_flushArea(platformXtmp, g_yres - 8, 12, 8);
		
		platformX = platformXtmp;            		
	}
}

// 发送设备数据（直接发送原始数据，不做字节序转换）
static void SendDeviceData(struct input_data *data) {
    // 数据格式：[1字节类型标识] + [设备数据(dev+val)]
    uint8_t send_buf[1 + sizeof(struct input_data)];
    
    // 设置数据类型
    send_buf[0] = DATA_TYPE_DEVICE;
    send_buf[1] = data->val ;
    // 直接复制原始数据（不做字节序转换）
//    memcpy(&send_buf[1], data, sizeof(struct input_data));
    
    // 发送数据
        NRF24L01_TxPacket[0] = send_buf[0];  // 第一个字节
        NRF24L01_TxPacket[1] = send_buf[1];  // 第二个字节
//  			LCD_PrintSignedVal (1,1, NRF24L01_TxPacket[1] );	
//              vTaskDelay (20);	
	// 3. 调用NRF24L01核心发送函数，触发发送
    uint8_t sendStatus = NRF24L01_Send();
 	LCD_PrintSignedVal (1,1, sendStatus );
	 
}

// 发送Key值（用于菜单选择）
static void SendKeyData(KeyEvent key) {
    // 数据格式：[1字节类型标识] + [1字节Key值]
    uint8_t send_buf[2];
    
    send_buf[0] = DATA_TYPE_KEY;  // 标识为Key值
    send_buf[1] = (uint8_t)key;   // Key值内容
    
    // 发送数据
         NRF24L01_TxPacket[0] = DATA_TYPE_KEY;  // 第一个字节
        NRF24L01_TxPacket[1] = send_buf[1];  // 第二个字节
 
	// 3. 调用NRF24L01核心发送函数，触发发送
    uint8_t sendStatus = NRF24L01_Send();
    
 
	
}

 // 发送任务：处理设备数据和Key值发送
void vSendPlatformDataTask() {
    struct input_data input ;
	 
    KeyEvent key;
    // Key队列轮询超时（短超时确保设备数据不会等待过久）
//    const TickType_t xKeyQueueTimeout = pdMS_TO_TICKS(10);
    
//    // 等待队列初始化完成
//    while (g_xQueuePlatform == NULL || g_xQueueKey == NULL) {
//        vTaskDelay( 100);
//    }
    
 
        // 优先处理Key值发送（菜单操作优先级更高）
        if (xQueueReceive(g_xQueueKey, &key, 0) == pdPASS) {
	//		  LCD_PrintString (1,1,"123");
            SendKeyData(key);

        }
        // 处理设备数据发送
          if (xQueueReceive(g_xQueuePlatform, &input, 0) == pdPASS) {
         
			SendDeviceData(&input);	
 			LCD_PrintSignedVal (1,5,NRF24L01_TxPacket[1] );	
              vTaskDelay (20);			  
        }
    
}
/**********************************************************************
 * 函数名称： ProcessIRData
 * 功能描述： 读取红外遥控器键值并转换为游戏控制键,写入挡球板队列
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期：      版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/09/02	     V1.0	  韦东山	      创建
 ***********************************************************************/
static void ProcessIRData(void)
{
	struct ir_data idata;
	static struct input_data input;
	
	xQueueReceive(g_xQueueIR, &idata, 0);
	
	if (idata.val == IR_KEY_LEFT)
	{
		input.dev = idata.dev;
		input.val = UPT_MOVE_LEFT;
	}
	else if (idata.val == IR_KEY_RIGHT)
	{
		input.dev = idata.dev;
		input.val = UPT_MOVE_RIGHT;
	}
	else if (idata.val == IR_KEY_REPEAT)
	{
		/* 保持不变 */;
	}
	else
	{
		input.dev = idata.dev;
		input.val = UPT_MOVE_NONE;
	}
	
	/* 写挡球板队列 */
	xQueueSend(g_xQueuePlatform, &input, 0);
}

/**********************************************************************
 * 函数名称： ProcessRotaryData
 * 功能描述： 读取旋转编码器数据并转换为游戏控制键,写入挡球板队列
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期：      版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/09/02	     V1.0	  韦东山	      创建
 ***********************************************************************/
static void ProcessRotaryData(void)
{
	struct rotary_data rdata;
	struct input_data idata;
	int left;
	int i, cnt;
	
	/* 读旋转编码器队列 */
	xQueueReceive(g_xQueueRotary, &rdata, 0);
			
	/* 处理数据 */
	/* 判断速度: 负数表示向左转动, 正数表示向右转动 */
	if (rdata.speed < 0)
	{
		left = 1;
		rdata.speed = 0 - rdata.speed;
	}
	else
	{
		left = 0;
	}
	
	//cnt = rdata.speed / 10;
	//if (!cnt)
	//	cnt = 1;
	if (rdata.speed > 100)
		cnt = 4;
	else if (rdata.speed > 50)
		cnt = 2;
	else
		cnt = 1;
			
	/* 写挡球板队列 */
	idata.dev = 1;
	idata.val = left ? UPT_MOVE_LEFT : UPT_MOVE_RIGHT;
	for (i = 0; i < cnt; i++)
	{
//		xQueueSend(g_xQueuePlatform, &idata, 0);
	}
}

/**********************************************************************
 * 函数名称： ProcessMPU6050Data
 * 功能描述： 读取MPU6050D的角度值并转换为游戏控制键,写入挡球板队列
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期：      版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/09/05	     V1.0	  韦东山	      创建
 ***********************************************************************/
static void ProcessMPU6050Data(void)
{
	struct mpu6050_data mdata;
	struct input_data idata;
	
	/* 读旋转编码器队列 */
	xQueueReceive(g_xQueueMPU6050, &mdata, 0);
			
	/* 处理数据 */
	/* 判断角度, 大于90度表示往左移动挡球板, 小于90度表示往右移动挡球板 */
	if (mdata.angle_y > 90)
	{
		idata.val = UPT_MOVE_LEFT;
	}
	else if(mdata.angle_y < 90)
	{
		idata.val = UPT_MOVE_RIGHT;
	}
	else
	{
		idata.val = UPT_MOVE_NONE;
	}
	
	/* 写挡球板队列 */
	idata.dev = 2;
	xQueueSend(g_xQueuePlatform, &idata, 0);
//     LCD_PrintSignedVal (1,1,idata .val );
//	vTaskDelay (10);
}
 void vKeyScanTask() {
    KeyEvent key_event = KEY_NONE;
	 static struct input_data input;
    // 用于消抖的时间标记
    TickType_t last_debounce_time = 0;
    const TickType_t debounce_delay = pdMS_TO_TICKS(50); // 消抖延时50ms
 //   last_debounce_time = xTaskGetTickCount();
   while (1) 
   {
        // 读取PA4（菜单左选）
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET) {
//			last_debounce_time = xTaskGetTickCount();
//            if ((xTaskGetTickCount() - last_debounce_time) > debounce_delay) {
//          
//                
//				
//            }   
			key_event = KEY_MENU_LEFT;
                xQueueSend(g_xQueueKey, &key_event, 0);
        }
        // 读取PA7（菜单右选）
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == GPIO_PIN_RESET) {
//			last_debounce_time = xTaskGetTickCount();
//            if ((xTaskGetTickCount() - last_debounce_time) > debounce_delay) {
//         
//                 
//            }
			key_event = KEY_MENU_RIGHT;
                xQueueSend(g_xQueueKey, &key_event, 0);
        }
        // 读取PA6（菜单返回）
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_RESET) {
//			last_debounce_time = xTaskGetTickCount();
//            if ((xTaskGetTickCount() - last_debounce_time) > debounce_delay) {

//                 
//            }
			key_event = KEY_MENU_BACK;
                xQueueSend(g_xQueueKey, &key_event, 0);
        }
        // 读取PB3（菜单确认）
        else if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_RESET) {
//			last_debounce_time = xTaskGetTickCount();
//            if ((xTaskGetTickCount() - last_debounce_time) > debounce_delay) {

//                 
//            } 
			key_event = KEY_MENU_CONFIRM;
                xQueueSend(g_xQueueKey, &key_event, 0);
        }
        // 读取PB4（游戏左移）
       else   if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_RESET) 
			{
//			last_debounce_time = xTaskGetTickCount();
//            if ((xTaskGetTickCount() - last_debounce_time) > debounce_delay) {
////                key_event = KEY_GAME_LEFT;

//                
//                
//				 
//             }
			input.dev=3;
			input .val=UPT_MOVE_LEFT;
            xQueueSend(g_xQueuekey_game, &input , 0);
			 
        }
        // 读取PA15（游戏右移）
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET) {
//			last_debounce_time = xTaskGetTickCount();
//            if ((xTaskGetTickCount() - last_debounce_time) > debounce_delay) 
//				{
////                key_event = KEY_GAME_RIGHT;
				input.dev=3;
				input .val=UPT_MOVE_RIGHT;
                 xQueueSend(g_xQueuekey_game, &input , 0);
//                 
 			 	 
//            }
         }
	
        vTaskDelay(pdMS_TO_TICKS(10)); // 降低扫描频率，减轻CPU负担
    } 
} 
/**********************************************************************
 * 函数名称： InputTask
 * 功能描述： 输入任务,检测多个输入设备并调用对应处理函数
 * 输入参数： params - 未使用
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期：      版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/09/02	     V1.0	  韦东山	      创建
 ***********************************************************************/
static void InputTask(void *params)
{
	QueueSetMemberHandle_t xQueueHandle;
     
 
	while (1)
	{
		/* 读队列集, 得到有数据的队列句柄 */
		xQueueHandle = xQueueSelectFromSet(g_xQueueSetInput, portMAX_DELAY);
		
		if (xQueueHandle)
		{
			/* 读队列句柄得到数据,处理数据 */
			if (xQueueHandle == g_xQueuekey_game)
			{
				struct input_data mdata;
	          struct input_data idata;
  
	       xQueueReceive(g_xQueuekey_game, &mdata, 0);
			idata.dev = 3;
           idata.val=mdata.val;
     
	        xQueueSend(g_xQueuePlatform, &idata, 0);	 
			}
//			else if (xQueueHandle == g_xQueueRotary)
//			{
//				ProcessRotaryData();
//			}			
			  if (xQueueHandle == g_xQueueMPU6050)
			{
				ProcessMPU6050Data();
			}			
		}
	}
}

 
void game1_task()
	{	
 
 
	 g_xQueuekey_game =xQueueCreate(10, sizeof(struct input_data));
	g_xQueueKey = xQueueCreate(10, sizeof(KeyEvent));	
		
	g_xQueuePlatform = xQueueCreate(10, sizeof(struct input_data));
	
	
		g_xQueueSetInput = xQueueCreateSet(20);
 
	g_xQueueMPU6050 = GetQueueMPU6050();
	
	xQueueAddToSet(g_xQueuekey_game, g_xQueueSetInput);
//	xQueueAddToSet(g_xQueueRotary, g_xQueueSetInput);
	xQueueAddToSet(g_xQueueMPU6050, g_xQueueSetInput);
	
	 xTaskCreate(vKeyScanTask, "KeyScanTask", 128, NULL, osPriorityNormal, NULL);
    xTaskCreate(MPU6050_Task, "MPU6050Task", 128, NULL, osPriorityNormal, NULL);
		
    xTaskCreate(InputTask, "InputTask", 128, NULL, osPriorityNormal, NULL);

 
//xTaskCreate(vSendPlatformDataTask, "SendPlatformDataTask", 128, g_xQueuePlatform, osPriorityNormal, NULL);


    while (1)
    {
		
 		vSendPlatformDataTask();
  // MPU6050_Test();
        vTaskDelay(50);
    }
}
