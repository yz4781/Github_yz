#ifndef __NRF24L01_H
#define __NRF24L01_H

#include "main.h"
 
#include "gpio.h"
 
#include "NRF24L01_Define.h"
 
 

/* 引脚定义（根据实际硬件修改） */
#define NRF24L01_CE_GPIO_Port    GPIOB
#define NRF24L01_CE_Pin          GPIO_PIN_9

#define NRF24L01_CSN_GPIO_Port   GPIOB  // CS: PB15
#define NRF24L01_CSN_Pin         GPIO_PIN_15

#define NRF24L01_SCK_GPIO_Port   GPIOB  // SPI1_SCK: PB14
#define NRF24L01_SCK_Pin         GPIO_PIN_14

#define NRF24L01_MOSI_GPIO_Port  GPIOB  // SPI1_MOSI: PB12
#define NRF24L01_MOSI_Pin        GPIO_PIN_12

#define NRF24L01_MISO_GPIO_Port  GPIOB  // SPI1_MISO: PB13
#define NRF24L01_MISO_Pin        GPIO_PIN_13

 // 接收通道分配：发送端1 → 接收通道0，发送端2 → 接收通道1
#define NRF24L01_CHANNEL_0  0  // 对应发送端1的接收通道
#define NRF24L01_CHANNEL_1  1  // 对应发送端2的接收通道

// 两个发送端的接收地址（需与发送端的TxAddress完全一致！）
static uint8_t NRF24L01_RxAddr_Ch0[5] = {0xA1,0xA2,0xA3,0xA4,0xA5};  // 发送端1的地址
static uint8_t NRF24L01_RxAddr_Ch1[5] = {0xB1,0xB2,0xB3,0xB4,0xB5};  // 发送端2的地址（与Ch0不同）

// 两个发送端对应的数据缓存区（独立缓存，避免数据覆盖）
#define NRF24L01_RX_PACKET_WIDTH  2  // 数据包宽度（与发送端一致，原代码保持不变）
//     uint8_t NRF24L01_RxData_Ch0[NRF24L01_RX_PACKET_WIDTH] = {3,8};  // 发送端1的数据缓存
//     uint8_t NRF24L01_RxData_Ch1[NRF24L01_RX_PACKET_WIDTH] = {1,1};  // 发送端2的数据缓存

// 数据来源标识（用于上层判断数据来自哪个发送端）
typedef enum {
    NRF24L01_NO_DATA = 0,  // 无数据
    NRF24L01_DATA_FROM_CH0, // 数据来自发送端1（通道0）
    NRF24L01_DATA_FROM_CH1  // 数据来自发送端2（通道1）
} NRF24L01_DataSrc;

/* 函数声明 */
uint8_t NRF24L01_ReadReg(uint8_t RegAddress);
void NRF24L01_ReadRegs(uint8_t RegAddress, uint8_t *DataArray, uint8_t Count);
void NRF24L01_WriteReg(uint8_t RegAddress, uint8_t Data);
void NRF24L01_WriteRegs(uint8_t RegAddress, uint8_t *DataArray, uint8_t Count);
void NRF24L01_ReadRxPayload(uint8_t *DataArray, uint8_t Count);
void NRF24L01_WriteTxPayload(uint8_t *DataArray, uint8_t Count);
void NRF24L01_FlushTx(void);
void NRF24L01_FlushRx(void);
uint8_t NRF24L01_ReadStatus(void);

void NRF24L01_PowerDown(void);
void NRF24L01_StandbyI(void);
void NRF24L01_Rx(void);
void NRF24L01_Tx(void);

void NRF24L01_Init(void);
uint8_t NRF24L01_Send(void);
NRF24L01_DataSrc NRF24L01_Receive(void);
void NRF24L01_GetRxData(uint8_t ch, uint8_t *data);
void NRF24L01_UpdateRxAddress(void);

#endif

