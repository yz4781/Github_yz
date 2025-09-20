#ifndef __NRF24L01_DEFINE_H
#define __NRF24L01_DEFINE_H

/* NRF24L01 指令定义 */
#define NRF24L01_R_REGISTER			0x00	// 读寄存器
#define NRF24L01_W_REGISTER			0x20	// 写寄存器
#define NRF24L01_R_RX_PAYLOAD		0x61	// 读Rx有效载荷
#define NRF24L01_W_TX_PAYLOAD		0xA0	// 写Tx有效载荷
#define NRF24L01_FLUSH_TX			0xE1	// 清空Tx FIFO
#define NRF24L01_FLUSH_RX			0xE2	// 清空Rx FIFO
#define NRF24L01_REUSE_TX_PL		0xE3	// 重用Tx有效载荷
#define NRF24L01_R_RX_PL_WID		0x60	// 读Rx有效载荷宽度
#define NRF24L01_W_ACK_PAYLOAD		0xA8	// 写ACK有效载荷
#define NRF24L01_W_TX_PAYLOAD_NOACK	0xB0	// 写Tx有效载荷（无ACK）
#define NRF24L01_NOP				0xFF	// 空操作

/* NRF24L01 寄存器地址定义 */
#define NRF24L01_CONFIG				0x00	// 配置寄存器
#define NRF24L01_EN_AA				0x01	// 使能自动应答
#define NRF24L01_EN_RXADDR			0x02	// 使能接收通道
#define NRF24L01_SETUP_AW			0x03	// 设置地址宽度
#define NRF24L01_SETUP_RETR			0x04	// 设置自动重传
#define NRF24L01_RF_CH				0x05	// 射频通道
#define NRF24L01_RF_SETUP			0x06	// 射频配置
#define NRF24L01_STATUS				0x07	// 状态寄存器
#define NRF24L01_OBSERVE_TX			0x08	// 发送观察寄存器
#define NRF24L01_RPD				0x09	// 接收功率检测
#define NRF24L01_RX_ADDR_P0			0x0A	// 接收通道0地址
#define NRF24L01_RX_ADDR_P1			0x0B	// 接收通道1地址
#define NRF24L01_RX_ADDR_P2			0x0C	// 接收通道2地址
#define NRF24L01_RX_ADDR_P3			0x0D	// 接收通道3地址
#define NRF24L01_RX_ADDR_P4			0x0E	// 接收通道4地址
#define NRF24L01_RX_ADDR_P5			0x0F	// 接收通道5地址
#define NRF24L01_TX_ADDR			0x10	// 发送地址
#define NRF24L01_RX_PW_P0			0x11	// 接收通道0有效载荷宽度
#define NRF24L01_RX_PW_P1			0x12	// 接收通道1有效载荷宽度
#define NRF24L01_RX_PW_P2			0x13	// 接收通道2有效载荷宽度
#define NRF24L01_RX_PW_P3			0x14	// 接收通道3有效载荷宽度
#define NRF24L01_RX_PW_P4			0x15	// 接收通道4有效载荷宽度
#define NRF24L01_RX_PW_P5			0x16	// 接收通道5有效载荷宽度
#define NRF24L01_FIFO_STATUS		0x17	// FIFO状态寄存器
#define NRF24L01_DYNPD				0x1C	// 使能动态有效载荷
#define NRF24L01_FEATURE			0x1D	// 使能高级功能

#endif




