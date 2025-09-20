#include "NRF24L01.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
///* 全局变量定义 */
//static  uint8_t NRF24L01_TxAddress[5] = {0xA1,0xA2,0xA3,0xA4,0xA5};
//#define NRF24L01_TX_PACKET_WIDTH		2							//发送数据包宽度，范围：1~32字节
//static uint8_t NRF24L01_TxPacket[NRF24L01_TX_PACKET_WIDTH] = {0};

//static uint8_t NRF24L01_RxAddress[5] = {0xA1,0xA2,0xA3,0xA4,0xA5};
//#define NRF24L01_RX_PACKET_WIDTH		2							//接收通道0数据包宽度，范围：1~32字节
//static uint8_t NRF24L01_RxPacket[NRF24L01_RX_PACKET_WIDTH] = {0};
     uint8_t NRF24L01_RxData_Ch0[NRF24L01_RX_PACKET_WIDTH] = {3,8};  // 发送端1的数据缓存
     uint8_t NRF24L01_RxData_Ch1[NRF24L01_RX_PACKET_WIDTH] = {1,1};  // 发送端2的数据缓存
typedef enum
{ Bit_RESET = 0,
  Bit_SET
}BitAction;

 
/* 引脚控制函数 */
static void NRF24L01_W_CE(uint8_t BitValue) {
    HAL_GPIO_WritePin(NRF24L01_CE_GPIO_Port, NRF24L01_CE_Pin, (BitAction)BitValue);
}

static void NRF24L01_W_CSN(uint8_t BitValue) {
    HAL_GPIO_WritePin(NRF24L01_CSN_GPIO_Port, NRF24L01_CSN_Pin, (BitAction)BitValue);
}
/* 引脚控制函数 */
static void NRF24L01_W_SCK(uint8_t BitValue) {
    HAL_GPIO_WritePin(NRF24L01_SCK_GPIO_Port, NRF24L01_SCK_Pin, (BitAction)BitValue);
}

static void NRF24L01_W_MOSI(uint8_t BitValue) {
    HAL_GPIO_WritePin(NRF24L01_MOSI_GPIO_Port, NRF24L01_MOSI_Pin, (BitAction)BitValue);
}
static  uint8_t  NRF24L01_R_MISO(void ) {
   return  HAL_GPIO_ReadPin (NRF24L01_MISO_GPIO_Port, NRF24L01_MISO_Pin );
}
/* SPI通信函数 - 使用SPI1句柄 */
uint8_t NRF24L01_SPI_SwapByte(uint8_t Byte)
{
	uint8_t i;
	
	/*此处使用SPI模式0进行通信*/
	/*循环8次，主机依次移出和移入数据的每一位*/
	for (i = 0; i < 8; i ++)
	{
		/*SPI为高位先行，因此移出高位至MOSI引脚*/
		if (Byte & 0x80)			//判断Byte的最高位
		{
			NRF24L01_W_MOSI(1);		//如果为1，则给MOSI输出1
		}
		else
		{
			NRF24L01_W_MOSI(0);		//如果为0，则给MOSI输出0
		}
		Byte <<= 1;					//Byte左移一位，最低位空出来用于接收数据位
		
		/*产生SCK上升沿*/
		NRF24L01_W_SCK(1);
		
		/*从MISO引脚移入数据，存入Byte的最低位*/
		if (NRF24L01_R_MISO())		//读取MISO引脚
		{
			Byte |= 0x01;			//如果为1，则给Byte最低位置1
		}							//如果为0，则不做任何操作，因为左移后低位默认补0
		
		/*产生SCK下降沿*/
		NRF24L01_W_SCK(0);
	}
	
	/*返回Byte数据，此时的Byte为SPI交换接收得到的一个字节数据*/
	return Byte;
}

/* 基础指令函数实现 */
uint8_t NRF24L01_ReadReg(uint8_t RegAddress) {
    uint8_t Data = 0;
    NRF24L01_W_CSN(0);
    NRF24L01_SPI_SwapByte(NRF24L01_R_REGISTER | RegAddress);
    Data = NRF24L01_SPI_SwapByte(NRF24L01_NOP);
    NRF24L01_W_CSN(1);
    return Data;
}

void NRF24L01_ReadRegs(uint8_t RegAddress, uint8_t *DataArray, uint8_t Count)
{
	uint8_t i;
	
	/*CSN置低，通信开始*/
	NRF24L01_W_CSN(0);
	
	/*交换发送一个字节，通信开始的第一个字节为指令码，读寄存器（低5位为寄存器地址）*/
	NRF24L01_SPI_SwapByte(NRF24L01_R_REGISTER | RegAddress);
	
	/*发送读寄存器指令后，开始交换接收，循环接收多次，得到指定地址下的多个数据*/
	for (i = 0; i < Count; i ++)
	{
		/*将接收到的数据写入到输出参数DataArray中*/
		DataArray[i] = NRF24L01_SPI_SwapByte(NRF24L01_NOP);
	}
	
	/*CSN置高，通信结束*/
	NRF24L01_W_CSN(1);
}

void NRF24L01_WriteReg(uint8_t RegAddress, uint8_t Data) {
    NRF24L01_W_CSN(0);
    NRF24L01_SPI_SwapByte(NRF24L01_W_REGISTER | RegAddress);
    NRF24L01_SPI_SwapByte(Data);
    NRF24L01_W_CSN(1);
}

void NRF24L01_WriteRegs(uint8_t RegAddress, uint8_t *DataArray, uint8_t Count)
{
	uint8_t i;
	
	/*CSN置低，通信开始*/
	NRF24L01_W_CSN(0);
	
	/*交换发送一个字节，通信开始的第一个字节为指令码，写寄存器（低5位为寄存器地址）*/
	NRF24L01_SPI_SwapByte(NRF24L01_W_REGISTER | RegAddress);
	
	/*发送写寄存器指令后，开始交换发送，循环发送多次，在指定地址下写入多个数据*/
	for (i = 0; i < Count; i ++)
	{
		/*将输入参数DataArray的数据写入到指定地址中*/
		NRF24L01_SPI_SwapByte(DataArray[i]);
	}
	
	/*CSN置高，通信结束*/
	NRF24L01_W_CSN(1);
}

void NRF24L01_ReadRxPayload(uint8_t *DataArray, uint8_t Count)
{
	uint8_t i;
	
	/*CSN置低，通信开始*/
	NRF24L01_W_CSN(0);
	
	/*交换发送一个字节，通信开始的第一个字节为指令码，读取Rx有效载荷*/
	NRF24L01_SPI_SwapByte(NRF24L01_R_RX_PAYLOAD);
	
	/*发送读取Rx有效载荷指令后，开始交换接收，循环接收多次，得到多个数据*/
	for (i = 0; i < Count; i ++)
	{
		/*将读取的数据写入到输出参数DataArray中*/
		DataArray[i] = NRF24L01_SPI_SwapByte(NRF24L01_NOP);
	}
	
	/*CSN置高，通信结束*/
	NRF24L01_W_CSN(1);
}
void NRF24L01_WriteTxPayload(uint8_t *DataArray, uint8_t Count)
{
	uint8_t i;
	
	/*CSN置低，通信开始*/
	NRF24L01_W_CSN(0);
	
	/*交换发送一个字节，通信开始的第一个字节为指令码，写入Tx有效载荷*/
	NRF24L01_SPI_SwapByte(NRF24L01_W_TX_PAYLOAD);
	
	/*发送写入Tx有效载荷指令后，开始交换发送，循环发送多次，写入多个数据*/
	for (i = 0; i < Count; i ++)
	{
		/*将输入参数DataArray的数据写入到Tx有效载荷中*/
		NRF24L01_SPI_SwapByte(DataArray[i]);
	}
	
	/*CSN置高，通信结束*/
	NRF24L01_W_CSN(1);
}
void NRF24L01_FlushTx(void) {
    NRF24L01_W_CSN(0);
    NRF24L01_SPI_SwapByte(NRF24L01_FLUSH_TX);
    NRF24L01_W_CSN(1);
}

void NRF24L01_FlushRx(void) {
    NRF24L01_W_CSN(0);
    NRF24L01_SPI_SwapByte(NRF24L01_FLUSH_RX);
    NRF24L01_W_CSN(1);
}

uint8_t NRF24L01_ReadStatus(void) {
    uint8_t Status;
    NRF24L01_W_CSN(0);
    Status = NRF24L01_SPI_SwapByte(NRF24L01_NOP);
    NRF24L01_W_CSN(1);
    return Status;
}

/* 工作模式函数实现 */
void NRF24L01_PowerDown(void) {
    uint8_t Config;
    NRF24L01_W_CE(0);
    Config = NRF24L01_ReadReg(NRF24L01_CONFIG);
    if (Config == 0xFF) return;
    Config &= ~0x02;
    NRF24L01_WriteReg(NRF24L01_CONFIG, Config);
}

void NRF24L01_StandbyI(void) {
    uint8_t Config;
    NRF24L01_W_CE(0);
    Config = NRF24L01_ReadReg(NRF24L01_CONFIG);
    if (Config == 0xFF) return;
    Config |= 0x02;
    NRF24L01_WriteReg(NRF24L01_CONFIG, Config);
}

void NRF24L01_Rx(void) {
    uint8_t Config;
    NRF24L01_W_CE(0);
    Config = NRF24L01_ReadReg(NRF24L01_CONFIG);
    if (Config == 0xFF) return;
    Config |= 0x03;
    NRF24L01_WriteReg(NRF24L01_CONFIG, Config);
    NRF24L01_W_CE(1);
}

void NRF24L01_Tx(void) {
    uint8_t Config;
    NRF24L01_W_CE(0);
    Config = NRF24L01_ReadReg(NRF24L01_CONFIG);
    if (Config == 0xFF) return;
    Config |= 0x02;
    Config &= ~0x01;
    NRF24L01_WriteReg(NRF24L01_CONFIG, Config);
    NRF24L01_W_CE(1);
}

/* 核心功能函数实现 */
void NRF24L01_Init(void) {
/*置引脚初始化后的默认电平*/
	NRF24L01_W_CE(0);		//CE默认为0，退出收发模式
	NRF24L01_W_CSN(1);		//CSN默认为1，不选中从机
	NRF24L01_W_SCK(0);		//SCK默认为0，对应SPI模式0
	NRF24L01_W_MOSI(0);		//MOSI默认电平随意，1和0均可
    
    /* 寄存器配置 */
    NRF24L01_WriteReg(NRF24L01_CONFIG, 0x0F);
    NRF24L01_WriteReg(NRF24L01_EN_AA, 0x03);
    NRF24L01_WriteReg(NRF24L01_EN_RXADDR, 0x03);
    NRF24L01_WriteReg(NRF24L01_SETUP_AW, 0x03);
    NRF24L01_WriteReg(NRF24L01_SETUP_RETR, 0x03);
    NRF24L01_WriteReg(NRF24L01_RF_CH, 0x02);
    NRF24L01_WriteReg(NRF24L01_RF_SETUP, 0x0E);
    
    NRF24L01_WriteReg(NRF24L01_RX_PW_P0, NRF24L01_RX_PACKET_WIDTH);
    NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_RxAddr_Ch0, 5);
    NRF24L01_WriteReg(NRF24L01_RX_PW_P1, NRF24L01_RX_PACKET_WIDTH);
    NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P1, NRF24L01_RxAddr_Ch1, 5); 	
    
    NRF24L01_FlushTx();
     NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_STATUS, 0x70);
    
    NRF24L01_Rx();
}
 

NRF24L01_DataSrc NRF24L01_Receive(void) {
    uint8_t Status = 0;
    uint8_t Config = 0;
    uint8_t RxCh = 0;  // 接收通道号（0或1）

    /* 1. 读取状态寄存器和配置寄存器，判断是否处于接收模式且有数据 */
    Status = NRF24L01_ReadStatus();
    Config = NRF24L01_ReadReg(NRF24L01_CONFIG);

    // 检查1：是否处于断电模式（若未初始化，重新初始化）
    if ((Config & 0x02) == 0x00) {
        NRF24L01_Init();
        return NRF24L01_NO_DATA;
    }

    // 检查2：是否有接收数据中断（RX_DR标志，bit6=1表示有数据）
    if ((Status & 0x40) == 0x40) {
        /* 2. 提取RX_P_NO位（bit3~bit1），确定数据来自哪个通道 */
        RxCh = (Status >> 1) & 0x07;  // 右移1位，取bit3~bit1（值为0表示通道0，1表示通道1）

        /* 3. 根据通道号，读取对应缓存区的数据 */
        switch (RxCh) {
            case NRF24L01_CHANNEL_0:  // 数据来自发送端1（通道0）
 
        	
                NRF24L01_ReadRxPayload(NRF24L01_RxData_Ch0, NRF24L01_RX_PACKET_WIDTH);
 
			  
                break;
            case NRF24L01_CHANNEL_1:  // 数据来自发送端2（通道1）
                NRF24L01_ReadRxPayload(NRF24L01_RxData_Ch1, NRF24L01_RX_PACKET_WIDTH);
               LCD_PrintSignedVal(1, 1, NRF24L01_RxData_Ch0[1]);
                break;
            default:  // 通道2-5未启用，忽略
                NRF24L01_FlushRx();  // 清空无效数据
                NRF24L01_WriteReg(NRF24L01_STATUS, 0x40);  // 清除RX_DR标志
                return NRF24L01_NO_DATA;
			break ;
        }

        /* 4. 清除接收中断标志，准备下一次接收 */
       NRF24L01_WriteReg(NRF24L01_STATUS, 0x40);  // 写1清除RX_DR标志
        NRF24L01_FlushRx();  // 清空Rx FIFO（避免残留数据）
            LCD_PrintSignedVal(0, 0, RxCh); 
		
        /* 5. 返回数据来源 */
        return (RxCh == NRF24L01_CHANNEL_0) ? NRF24L01_DATA_FROM_CH0 : NRF24L01_DATA_FROM_CH1;
    } 
    else {
        // 无数据或其他错误（如MAX_RT），返回无数据
        if ((Status & 0x30) == 0x30) {  // 若出现最大重传错误，重新初始化
            NRF24L01_Init();
        }
        return NRF24L01_NO_DATA;
    }
}

/* 新增：获取对应通道的接收数据（上层调用此函数读取数据） */
void NRF24L01_GetRxData(uint8_t ch, uint8_t *data) {
    if (data == NULL) return;  // 防止空指针

    switch (ch) {
        case NRF24L01_CHANNEL_0:
			data[0] =NRF24L01_RxData_Ch0[0];
		    data[1] =NRF24L01_RxData_Ch0[1];
 //   		LCD_PrintSignedVal(1, 1, NRF24L01_RxData_Ch0[1]);
 //             memcpy(data, NRF24L01_RxData_Ch0, NRF24L01_RX_PACKET_WIDTH);  // 复制通道0数据
		/* 4. 清除接收中断标志，准备下一次接收 */
//        NRF24L01_WriteReg(NRF24L01_STATUS, 0x40);  // 写1清除RX_DR标志
//          NRF24L01_FlushRx();  // 清空Rx FIFO（避免残留数据）
		
            break;
        case NRF24L01_CHANNEL_1:
			data[0] =NRF24L01_RxData_Ch1[0];
		    data[1] =NRF24L01_RxData_Ch1[1];
//             memcpy(data, NRF24L01_RxData_Ch1, NRF24L01_RX_PACKET_WIDTH);  // 复制通道1数据
		/* 4. 清除接收中断标志，准备下一次接收 */
//        NRF24L01_WriteReg(NRF24L01_STATUS, 0x40);  // 写1清除RX_DR标志
//        NRF24L01_FlushRx();
            break;
        default:
//			data  = 0;
             memset(data, 0, NRF24L01_RX_PACKET_WIDTH);  // 无效通道，返回0
            break;
    }
}
//void NRF24L01_UpdateRxAddress(void)
//{
//	/*接收通道0地址，设置为全局数组NRF24L01_RxAddress指定的地址，地址宽度固定为5字节*/
//	NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_RxAddr_Ch0, 5);
//	NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P1, NRF24L01_RxAddr_Ch1, 5); 
//}
