/***************************************************************************************
* Copyright (c) 2024 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __UART16550_H
#define __UART16550_H

#include "common.h"

// 16550 UART Constants
#define CH_OFFSET 0
#define LSR_OFFSET 5
#define LSR_TX_READY 0x20
#define LSR_FIFO_EMPTY 0x40
#define LSR_RX_READY 0x01

// 16550 Register offsets
#define RBR_THR_OFFSET 0x00  // Receiver Buffer Register / Transmitter Holding Register
#define IER_OFFSET 0x01       // Interrupt Enable Register
#define IIR_FCR_OFFSET 0x02   // Interrupt Identification Register / FIFO Control Register
#define LCR_OFFSET 0x03       // Line Control Register
#define MCR_OFFSET 0x04       // Modem Control Register
#define LSR_OFFSET 0x05       // Line Status Register
#define MSR_OFFSET 0x06       // Modem Status Register
#define SCR_OFFSET 0x07       // Scratch Register

// FIFO sizes
#define RX_FIFO_SIZE 16
#define TX_FIFO_SIZE 16

// Main UART functions
extern "C" uint8_t uart16550_getc(void);
extern "C" void uart16550_getc_legacy(uint8_t *ch);
extern "C" void uart16550_putc(uint8_t ch);

// Register access functions
extern "C" uint8_t uart16550_read_reg(uint8_t offset);
extern "C" void uart16550_write_reg(uint8_t offset, uint8_t data);

// Initialization and cleanup
extern "C" void init_uart16550(void);
extern "C" void finish_uart16550(void);

// Debug functions
extern "C" void uart16550_get_fifo_status(int *rx_count, int *tx_count);

#endif // __UART16550_H 