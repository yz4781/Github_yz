 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <main.h> 

#include "cmsis_os.h"
#include "FreeRTOS.h"                   
#include "task.h"                        
#include "event_groups.h"               
#include "semphr.h"                    
#include "adc.h"
#include "draw.h"
#include "resources.h"
#include "driver_passive_buzzer.h" 
#include "driver_oled.h"
#include "driver_lcd.h"
#include "driver_ir_receiver.h"
#include "driver_rotary_encoder.h"
#include "driver_mpu6050.h"
 #include "NRF24L01.h" 

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
 extern  uint8_t NRF24L01_RxData_Ch0[NRF24L01_RX_PACKET_WIDTH]  ;  // 发送端1的数据缓存
 extern  uint8_t NRF24L01_RxData_Ch1[NRF24L01_RX_PACKET_WIDTH]  ;  // 发送端2的数据缓存
extern void PlayMusic(void *params);
void car_game_draw(void);
int g_game_running =0;
 
uint8_t data_channel;
// 自定义汽车图像（5字节，对应128×64屏的一列像素）
static const uint8_t car_img[] = {0x2E, 0xFF, 0xFF, 0xFF, 0x2E}; 
static const uint8_t car_clear_img[5] = {0};
// 障碍物图像（简化为单字节，可根据需求扩展）
static const uint8_t obstacle_img[] = {0x7E, 0x7F, 0x7F, 0x7F, 0x7E};
static const uint8_t obstacle_clear_img[5] = {0};
// 玩家汽车结构体（双玩家区分）
typedef struct {
    int x;          // X坐标
    int y;          // Y坐标
    int speed;      // 移动速度
} PlayerCar;

// 障碍物结构体
typedef struct {
    int x;          // X坐标
    int y;          // Y坐标
    uint8_t active; // 是否激活（1=显示，0=隐藏）
    int speed;      // 移动速度（随局数增加）
} Obstacle;

// 游戏全局状态（双局独立+全局胜负）
typedef struct {
    PlayerCar car;                  // 玩家汽车
    Obstacle obstacles[5];          // 每个玩家5个障碍物（避免过多）
    int score;                      // 玩家单局得分（躲避障碍物计数）
    bool game_over;                 // 当前局是否结束
    int obstacle_speed;             // 当前局障碍物移动速度
} GameState;

// 全局变量（双玩家+全局胜负）
GameState player1;  // 玩家1（OLED上半屏）
GameState player2;  // 玩家2（OLED下半屏）
int win_count1 = 0; // 玩家1胜局数
int win_count2 = 0; // 玩家2胜局数
int current_round = 1; // 当前局数（1~5）
 
  

static   xQueueHandle g_xQueueKey1;
static   xQueueHandle g_xQueueKey2;
 typedef enum {
    KEY_NONE = 0,
    KEY_MENU_LEFT,   // PB1：菜单左选
    KEY_MENU_RIGHT,  // PB2：菜单右选
    KEY_MENU_BACK,   // PB3：菜单返回
    KEY_MENU_CONFIRM,// PB4：菜单确认
 
} KeyEvent;
 
#define DATA_TYPE_DEVICE 0x00  // 设备数据标识
#define DATA_TYPE_KEY 0x01     // Key值标识

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
static QueueHandle_t g_xQueuePlatform1;  
static QueueHandle_t g_xQueuePlatform2;  
static QueueHandle_t g_xQueueIR;
static QueueHandle_t g_xQueueRotary;
static QueueHandle_t g_xQueueMPU6050; /* MPU6050队列 */
static   TaskHandle_t xSoundTaskHandle = NULL;
BaseType_t ret;
int32_t    addr,writeFiashData;
// 初始化单个玩家的游戏状态
void writeFiashTest(void )
{
	HAL_FLASH_Unlock ();
	FLASH_EraseInitTypeDef f;
	 f .TypeErase=FLASH_TYPEERASE_PAGES ;
	f.PageAddress=addr;
	f.NbPages=1;
	uint32_t PageErrror=0;
	
	HAL_FLASHEx_Erase (&f,&PageErrror);
	
	HAL_FLASH_Program (FLASH_TYPEPROGRAM_WORD ,addr,writeFiashData);
	
	HAL_FLASH_Lock ();

}

void game_init_single(GameState *player, int x_pos, int init_obstacle_speed) {
 
    player->car.x =x_pos ; // 汽车宽5像素， 
    player->car.y = (g_yres-8)/2+10;            
    player->car.speed = 2;            // 移动速度（固定）
    draw_bitmap(player->car.x, player->car.y, car_img, 5, 8, NOINVERT, 0);
	draw_flushArea(player->car.x, player->car.y, 5, 8);
    // 初始化障碍物（全部隐藏）
    for (int i = 0; i < 5; i++) {
        player->obstacles[i].active = 0;
        player->obstacles[i].speed = init_obstacle_speed; // 随局数设置初始速度
    }
    
    player->score = 0;
    player->game_over = false;
    player->obstacle_speed = init_obstacle_speed;
}

// 全局游戏初始化（每局开始时调用）
void car_game_init(void) {
    // 计算当前局障碍物初始速度：局数1=1，局数2=2，...，局数5=5（每局+1）
    int init_speed = current_round;
    if (init_speed > 3) init_speed = 3; // 上限5（避免过快）
    
    // 玩家1：上半屏（Y=20），玩家2：下半屏（Y=44）
    game_init_single(&player1, 30, init_speed);
    game_init_single(&player2, 94, init_speed);
      
    // 清空OLED屏幕
 //   LCD_Clear();  
}
 

 // 生成单个玩家的障碍物
void spawn_obstacle_single(GameState *player) {
    if (player->game_over) return; // 局结束则不生成
    
    for (int i = 0; i < 5; i++) {
        if (!player->obstacles[i].active) {
        // 分屏X范围：玩家1=0~63，玩家2=64~127
            int x_min = (player == &player1) ? 0 : 64;
            int x_max = (player == &player1) ? 63 : 127;
            player->obstacles[i].x = x_min + (rand() % (x_max - x_min + 1));
			player->obstacles[i].y =0;
            player->obstacles[i].active = 1;
            player->obstacles[i].speed = player->obstacle_speed; // 随局数更新速度
            break;
        }
    }
}

// 移动单个玩家的障碍物
void move_obstacles_single(GameState *player) {
    if (player->game_over) return;
        //   清除上一帧障碍物
    for (int i = 0; i < 5; i++) {
        if (player->obstacles[i].active) {
            draw_bitmap(player->obstacles[i].x, player->obstacles[i].y, obstacle_clear_img, 5, 8, NOINVERT, 0);
			 
	      draw_flushArea(player->obstacles[i].x,player->obstacles[i].y,  5, 8);
        }
    }
    for (int i = 0; i < 5; i++) {
        if (player->obstacles[i].active) {
            // 向下移动（速度随局数增加）
            player->obstacles[i].y += player->obstacles[i].speed;
            
//            // 超出玩家区域则隐藏+得分
//            int max_x = (player == &player1) ? 59 : 123; // 玩家1上限x=59（不越过分屏线）
            if (player->obstacles[i].y >= g_yres) {
                player->obstacles[i].active = 0;
                player->score++;
				vTaskDelay (200);
            }
        }
    }
	    // . 绘制当前障碍物
    for (int i = 0; i < 5; i++)
	{
        if (player->obstacles[i].active) {
            draw_bitmap(player->obstacles[i].x, player->obstacles[i].y,    obstacle_img, 5, 8, NOINVERT, 0);
			draw_flushArea(player->obstacles[i].x,player->obstacles[i].y,  5, 8);
        }
    }
    
    // 概率生成新障碍物（每帧15%概率，避免过多）
    if (rand() % 100 < 15) {
        spawn_obstacle_single(player);
    }
}

// 全局障碍物处理（双局并行）
void update_obstacles(void) {
    if (win_count1 >= 3 || win_count2 >= 3) return;
    
    move_obstacles_single(&player1);
    move_obstacles_single(&player2);
}


 
// 单个玩家碰撞检测（汽车宽5px，高8px；障碍物5x8px）
bool check_collision_single(GameState *player) {
    for (int i = 0; i < 5; i++) {
        if (player->obstacles[i].active) {
            // 矩形碰撞判定
            if (player->obstacles[i].x >= player->car.x && 
                player->obstacles[i].x < player->car.x + 5 && 
                player->obstacles[i].y >= player->car.y && 
                player->obstacles[i].y < player->car.y + 8) {
                return true;
            }
        }
    }
    return false;
}

void draw_final_result(void);
void draw_round_result(  char *result, int x, int y) ;


// 全局碰撞检测与局胜负判断
void check_game_round_end(void) {
    if (win_count1 >= 3 || win_count2 >= 3)	return;
    
    bool p1_collide = check_collision_single(&player1);
    bool p2_collide = check_collision_single(&player2);
    
    if (p1_collide && p2_collide) {
 
		g_game_running =2;
		
        draw_string_P(PSTR("BOTH COLLIDE! RESTART"), false, 10, 30);
        
        vTaskDelay( 2000 ); // 延时2秒
		LCD_Clear(); // 刷新OLED显示
        car_game_init(); // 重新初始化当前局
		g_game_running =1;
// 		       if (xSoundTaskHandle != NULL)
//			  { 
//				  if(g_game_running==0)
//				  {
//					   vTaskResume(xSoundTaskHandle);
//					  g_game_running=2;
//					  
//				  }
// 
//			  }  
    } 
    else if (p1_collide) {
		g_game_running =2;
        // 玩家1碰撞：玩家2胜
        player1.game_over = true;
        win_count2++;
        draw_round_result("PLAYER 2 WIN!", 50, 30);
		 
    } 
    else if (p2_collide) {
		g_game_running =2;
        // 玩家2碰撞：玩家1胜
        player2.game_over = true;
        win_count1++;
        draw_round_result("PLAYER 1 WIN!", 50, 30);
		  
    }
}

// 显示局结果（延时后进入下一局）
void draw_round_result(  char *result, int x, int y) {
    // 清除当前界面，显示结果
    LCD_Clear();
    draw_string_P(PSTR("ROUND END!"), false, x, y-10);
    draw_string(result, false, x, y);
    draw_string_P(PSTR("NEXT ROUND IN 2S"), false, x-5, y+10);
	 vTaskDelay(pdMS_TO_TICKS(2000) );
   LCD_Clear();
    
    // 延时2秒，进入下一局
 
    current_round++;
    
    // 判断是否分胜负（五局三胜）
    if (win_count1 >= 3 || win_count2 >= 3) {
		g_game_running =0;
        draw_final_result(); // 显示最终胜负
    } else {
		   // 延时2秒，进入下一局
       vTaskDelay(pdMS_TO_TICKS(2000));
        car_game_init(); // 初始化下一局（障碍物速度增加）
		g_game_running =1;
    }
}

// 显示最终五局三胜结果
void draw_final_result(void) {
    LCD_Clear();
    if (win_count1 >= 3) {
        draw_string_P(PSTR(" WINNER: PLAYER 1!"), false, 1, 10);
    } else if (win_count2 >= 3){
        draw_string_P(PSTR(" WINNER: PLAYER 2!"), false, 1, 20);
    }
    draw_string_P(PSTR("SCORE: "), false, 20, 40);
    char score_buf[20];
    sprintf(score_buf, "P1:%d P2:%d", win_count1, win_count2);
    draw_string(score_buf, false, 60, 40);
     TickType_t debounce_time = 0;
     TickType_t last_debounce_time = 0;
    debounce_time = xTaskGetTickCount();
    
    // 游戏结束，无限循环
    while (1) {
		   NRF24L01_DataSrc data_src;  
  
	  data_src = NRF24L01_Receive(); 
        vTaskDelay(pdMS_TO_TICKS(1000));
		if( data_src ==0 )
			{
				if(( xTaskGetTickCount()-debounce_time)>pdMS_TO_TICKS(10000)   )
				{ 
					 OLED_ManualSetBrightness(0);
					if( data_src  )
				OLED_ManualSetBrightness(100);
				}
	         }
    }
} 






// 处理玩家1输入
void process_input_player1(struct  input_data idata) {
    if (player1.game_over) return; // 局结束则不响应

    switch (idata.val) {
        case UPT_MOVE_LEFT: // 玩家1左移
			//   清除上一帧汽车（用背景色覆盖）
    draw_bitmap(player1.car.x, player1.car.y, car_clear_img, 5, 8, NOINVERT, 0);
	draw_flushArea(player1.car.x, player1.car.y,  5, 8);
            if (player1.car.x > 0) {
                player1.car.x -= player1.car.speed;
            }
            break;
        case UPT_MOVE_RIGHT: // 玩家1右移
						//   清除上一帧汽车（用背景色覆盖）
    draw_bitmap(player1.car.x, player1.car.y, car_clear_img, 5, 8, NOINVERT, 0);
	draw_flushArea(player1.car.x, player1.car.y,  5, 8);
            if (player1.car.x < 64 - 5) { // 汽车宽5像素
                player1.car.x += player1.car.speed;
            }
            break;
        default:
            break;
		   
    draw_bitmap(player1.car.x, player1.car.y, car_img, 5, 8, NOINVERT, 0);
	draw_flushArea(player1.car.x, player1.car.y, 5, 8);
    }
}

// 处理玩家2输入
void process_input_player2(struct  input_data idata) {
    if (player2.game_over) return; // 局结束则不响应
       

    switch (idata.val) {
        case UPT_MOVE_LEFT: // 玩家2左移
    draw_bitmap(player2.car.x, player2.car.y, car_clear_img, 5, 8, NOINVERT, 0);
	draw_flushArea(player2.car.x, player2.car.y, 5, 8);
            if (player2.car.x > 64) {
                player2.car.x -= player2.car.speed;
            }
            break;
        case UPT_MOVE_RIGHT: // 玩家2右移
	 draw_bitmap(player2.car.x, player2.car.y, car_clear_img, 5, 8, NOINVERT, 0);
	draw_flushArea(player2.car.x, player2.car.y, 5, 8);
            if (player2.car.x < g_xres - 5) {
                player2.car.x += player2.car.speed;
            }
            break;
        default:
            break;
   draw_bitmap(player2.car.x, player2.car.y, car_img, 5, 8, NOINVERT, 0);
	draw_flushArea(player2.car.x, player2.car.y, 5, 8);
    }
}

// 全局输入处理（从队列读取按键）
void process_game_input(void) {
    if (win_count1 >= 3 || win_count2 >= 3) return; // 已分胜负则不响应
    
   struct  input_data idata1,idata2;
    if (xQueueReceive( g_xQueuePlatform1, &idata1, 0) == pdPASS)
		{
			process_input_player1(idata1);
//       LCD_PrintSignedVal(8, 0, idata1.val);
        }
    if (xQueueReceive( g_xQueuePlatform2, &idata2, 0) == pdPASS)
		{
			process_input_player2(idata2);
        }
}





/**********************************************************************
 * 函数名称： InputTask
 * 功能描述： 输入任务,检测多个输入设备并调用对应处理函数
 * 输入参数： params - 未使用
 * 输出参数： 无
 * 返 回 值： 无
 
 ***********************************************************************/
static void InputTask()
{
 
	static   uint8_t rx_data[2] = {0}; // 临时存储接收数据
      NRF24L01_DataSrc data_src; // 数据来源标识（哪个通道/发送端）
	  struct input_data idata1;
	  KeyEvent menu_key1;
	  struct input_data idata2;
	  KeyEvent menu_key2;
	  
 	
 	
	  data_src = NRF24L01_Receive();  
	  LCD_PrintSignedVal(0, 0, data_src); 
	  // 根据数据来源，读取对应通道的缓存数据
        switch (data_src) {
        case NRF24L01_DATA_FROM_CH0:
			  NRF24L01_GetRxData(0, rx_data);
 //		 LCD_PrintSignedVal(2, 0, rx_data[1]);
 
                // 处理来自设备A的数据
                // 示例：rx_data[0]、rx_data[1]、rx_data[2]、rx_data[3]为接收到的数据
				if(rx_data[0]==DATA_TYPE_KEY)
				{
					menu_key1=rx_data[1];
					
					LCD_PrintSignedVal(1, 5, menu_key1);
//					LCD_PrintString(1, 0, "Starting");
					xQueueSend(g_xQueueKey1, &menu_key1, 0);
					
				}
				else if(rx_data[0]==DATA_TYPE_DEVICE)
				{
					idata1.dev =0x01;
					idata1.val = rx_data[1];
					LCD_PrintSignedVal(2, 0, idata1.val);
					 
					vTaskDelay (10);
//					LCD_PrintString(3, 0, "Starting");
					xQueueSend(g_xQueuePlatform1, &idata1, 0);
					
					
					
				}
          
				break ;	
		case NRF24L01_DATA_FROM_CH1:
	         NRF24L01_GetRxData(1, rx_data);
 
                // 处理来自设备B的数据
                // 示例：rx_data[0]、rx_data[1]、rx_data[2]、rx_data[3]为接收到的数据
      				if(rx_data[0]==DATA_TYPE_KEY)
				{
					menu_key2=rx_data[1];
					
					LCD_PrintSignedVal(2, 0, menu_key2);
					xQueueSend(g_xQueueKey2, &menu_key2, 0);
				}
				else if(rx_data[0]==DATA_TYPE_DEVICE)
				{
					idata2.dev =0x01;
					idata2.val =	rx_data[1];
					LCD_PrintSignedVal(3, 0, idata2.val);
					xQueueSend(g_xQueuePlatform2, &idata2, 0);
				}
				 
            break ;
		  case NRF24L01_NO_DATA:	
		                         rx_data [1]=0;
        
								 vTaskDelay (10);   
							   
							  break ;
								

		
		}  
	
	 

	}

 // /**********************************************************************
//绘制函数（核心显示逻辑）
// 
// ***********************************************************************/
void car_game_task() {
     // 游戏主循环（20FPS，每50ms一次）
	
 
 
//        g_game_running = 1;
        process_game_input();    // 处理双人输入
        update_obstacles();      // 更新双局障碍物
        check_game_round_end();  // 检测碰撞与局结束
   //     car_game_draw();         // 绘制双界面
        
  //       vTaskDelay(pdMS_TO_TICKS(50));
 
}



// 绘制单个玩家的游戏界面
void draw_game_single(GameState *player,   char *label, int x_offset) {
    // 1. 清除上一帧汽车（用背景色覆盖）
    draw_bitmap(player->car.x, player->car.y, car_clear_img, 5, 8, NOINVERT, 0);
    
    // 2. 清除上一帧障碍物
    for (int i = 0; i < 5; i++) {
        if (player->obstacles[i].active) {
            draw_bitmap(player->obstacles[i].x, player->obstacles[i].y, 
                        obstacle_clear_img, 5, 8, NOINVERT, 0);
        }
    }
    
    // 3. 绘制玩家标签（如"P1"、"P2"）
    draw_string(label, false, x_offset, 1);
    
    // 4. 绘制当前得分
    char score_buf[8];
    sprintf(score_buf, "S:%d", player->score);
    draw_string(score_buf, false, x_offset+20, 10);
    
    // 5. 绘制当前汽车
    draw_bitmap(player->car.x, player->car.y, car_img, 5, 8, NOINVERT, 0);
    
    // 6. 绘制当前障碍物
    for (int i = 0; i < 5; i++)
	{
        if (player->obstacles[i].active) {
            draw_bitmap(player->obstacles[i].x, player->obstacles[i].y, 
                        obstacle_img, 5, 8, NOINVERT, 0);
        }
    }
}

// 全局游戏绘制（双界面并行）
void car_game_draw(void) {
    if (win_count1 >= 3 || win_count2 >= 3) return; // 已分胜负则不绘制
    
    // 绘制玩家1（左半屏，标签Y=5）
    draw_game_single(&player1, "P1", 1);
    // 绘制玩家2（下半屏，标签Y=37）
    draw_game_single(&player2, "P2", 64);
    
    // 绘制当前局数
    char round_buf[16];
    sprintf(round_buf, "ROUND: %d", current_round);
    draw_string(round_buf, false, (g_xres - 60)/2, 60); // 底部居中
    
    // 刷新OLED显示
    LCD_Flush();
}
void Music_task()
{
//		 
//       if (xSoundTaskHandle == NULL)
//			  {
 
//				  if( g_game_running == 1)
//				  {
//					 xTaskCreate(PlayMusic, "SoundTask", 128, NULL, osPriorityNormal , &xSoundTaskHandle);
// 				
//				  }
//			 
//			  }
			         
 
				  if(g_game_running==2)
				  {
					   vTaskSuspend(xSoundTaskHandle);
					   PassiveBuzzer_Control(0); /* 停止蜂鸣器 */
 
					  
				  }
 		   
 		  
				else  if(g_game_running==1)
				  {
					   vTaskResume(xSoundTaskHandle);
					 
					  
				  }
				  
          else  if(g_game_running==0)
				  {
					   vTaskDelete(xSoundTaskHandle);
					   PassiveBuzzer_Control(0); /* 停止蜂鸣器 */
					  
				  }
 
 
		 
 


}


void game1_task()
{		 
    NRF24L01_Init();
	g_framebuffer = LCD_GetFrameBuffer(&g_xres, &g_yres, &g_bpp);
	draw_init();
	draw_end();
  	car_game_init();
   g_xQueuePlatform1 = xQueueCreate(10, sizeof(struct input_data));
   g_xQueuePlatform2 = xQueueCreate(10, sizeof(struct input_data));
    
//	    draw_bitmap(player1.car.x, player1 .car.y, car_img, 5, 8, NOINVERT, 0);
//	draw_flushArea(player1.car.x, player1.car.y, 5, 8); 
//    xTaskCreate(InputTask, "InputTask", 128, NULL, osPriorityNormal, NULL);
    


 

//    xTaskCreate(Music_task, "Music_task", 128, NULL, osPriorityNormal, NULL);

    while (1)
    {
 
//		 static    uint8_t rx_data[2] = {1,7}; // 临时存储接收数据
//      NRF24L01_DataSrc data_src; // 数据来源标识（哪个通道/发送端）
//	  struct input_data idata1;
//	  KeyEvent menu_key1;
//	  struct input_data idata2;
//	  KeyEvent menu_key2;
//	  
//	  
//	  data_src = NRF24L01_Receive();  
//	  LCD_PrintSignedVal(3, 3, data_src); 
// 
//        if ( data_src==NRF24L01_DATA_FROM_CH0)
//		{   
// 		 NRF24L01_GetRxData(0, rx_data);
// 		//	memcpy(rx_data, NRF24L01_RxData_Ch0, NRF24L01_RX_PACKET_WIDTH);  
//			
// 		LCD_PrintSignedVal(2, 0, rx_data[0]); 
//			
//			
//		}
//		    if ( data_src==NRF24L01_DATA_FROM_CH1)
//		{   
// 		 NRF24L01_GetRxData(1, rx_data);
// 		//	memcpy(rx_data, NRF24L01_RxData_Ch0, NRF24L01_RX_PACKET_WIDTH);  
//			
// 		LCD_PrintSignedVal(2, 3, rx_data[1]); 
//			
//			
//		}
//		else LCD_Clear ();
 //		Music_task();
 	InputTask();
  	 	car_game_task();
 
         vTaskDelay(50);
    }
}

 
// void car_game_draw(void) 
//{
//    if (game_over)
//	{
//        draw_string_P(PSTR("GAME OVER"), false, 30, 30);
//        draw_string_P(PSTR("SCORE:"), false, 30, 40);
//        char score_str[4];
//        sprintf(score_str, "%d", score);
//        draw_string(score_str, false, 60, 40);
//        return;
//    }

//			// 1. 清除上一帧汽车
//		draw_bitmap(player_car.x, player_car.y, car_clear_img, 5, 8, NOINVERT, 0);
//		draw_flushArea(player_car.x, player_car.y, 5, 8);

//    // 2. 清除上一帧障碍物
//    for (int i = 0; i < 10; i++) 
//	{
//        if (obstacles[i].active) 
//			{
//				draw_bitmap(obstacles[i].x, obstacles[i].y, obstacle_clear_img, 5, 8, NOINVERT, 0);
//				draw_flushArea(obstacles[i].x, obstacles[i].y, 5, 8);
//            }
//    }

//    // 3. 移动障碍物并检测碰撞
//    move_obstacles();
//    if (check_collision())
//		{
//			game_over = true;
//			return;
//        }

//	// 4. 绘制当前汽车
//		draw_bitmap(player_car.x, player_car.y, car_img, 5, 8, NOINVERT, 0);
//		draw_flushArea(player_car.x, player_car.y, 5, 8);

//    // 5. 绘制当前障碍物
//    for (int i = 0; i < 10; i++) 
//	{
//        if (obstacles[i].active) 
//			{
//				draw_bitmap(obstacles[i].x, obstacles[i].y, obstacle_img, 5, 8, NOINVERT, 0);
//				draw_flushArea(obstacles[i].x, obstacles[i].y, 5, 8);
//			}
//    }

//			// 6. 绘制得分
//			char score_str[4];
//			sprintf(score_str, "%d", score);
//			draw_string(score_str, false, 10, 10);
//}






 