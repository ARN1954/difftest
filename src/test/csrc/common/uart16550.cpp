/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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

#include "uart16550.h"
#include "common.h"
#include "stdlib.h"

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

// FIFO sizes (16 bytes each like hardware implementation)
#define RX_FIFO_SIZE 16
#define TX_FIFO_SIZE 16

// RX FIFO for incoming data
static char rx_fifo[RX_FIFO_SIZE] = {};
static int rx_f = 0, rx_r = 0;

// TX FIFO for outgoing data (simulated)
static char tx_fifo[TX_FIFO_SIZE] = {};
static int tx_f = 0, tx_r = 0;

// 16550 Register state
static uint8_t IER = 0;   // Interrupt Enable Register
static uint8_t IIR = 1;   // Interrupt Identification Register (bit 0 = 1: no interrupt)
static uint8_t FCR = 0;   // FIFO Control Register
static uint8_t LCR = 0;   // Line Control Register
static uint8_t MCR = 0;   // Modem Control Register
static uint8_t MSR = 0;   // Modem Status Register
static uint8_t SCR = 0;   // Scratch Register

// RX FIFO management
static void uart16550_rx_enqueue(char ch) {
  int next = (rx_r + 1) % RX_FIFO_SIZE;
  if (next != rx_f) {
    // not full
    rx_fifo[rx_r] = ch;
    rx_r = next;
  }
}

// RX FIFO dequeue (like NEMU's serial_dequeue)
static int uart16550_rx_dequeue(void) {
  int k = 0xff; // Default value like NEMU
  if (rx_f != rx_r) {
    k = rx_fifo[rx_f];
    rx_f = (rx_f + 1) % RX_FIFO_SIZE;
  }
  return k;
}

// TX FIFO enqueue (for simulation)
static void uart16550_tx_enqueue(char ch) {
  int next = (tx_r + 1) % TX_FIFO_SIZE;
  if (next != tx_f) {
    // not full
    tx_fifo[tx_r] = ch;
    tx_r = next;
  }
}

// TX FIFO dequeue (for simulation)
static int uart16550_tx_dequeue(void) {
  int k = 0;
  if (tx_f != tx_r) {
    k = tx_fifo[tx_f];
    tx_f = (tx_f + 1) % TX_FIFO_SIZE;
  }
  return k;
}

// Check if RX FIFO has data
static int uart16550_rx_ready(void) {
  return (rx_f != rx_r) ? LSR_RX_READY : 0;
}

// Check if TX FIFO is empty
static int uart16550_tx_empty(void) {
  return (tx_f == tx_r) ? (LSR_TX_READY | LSR_FIFO_EMPTY) : 0;
}

// Get LSR (Line Status Register) value
static uint8_t uart16550_get_lsr(void) {
  return LSR_TX_READY | LSR_FIFO_EMPTY | uart16550_rx_ready();
}

// Main UART getc function for 16550
uint8_t uart16550_getc() {
  static uint32_t lasttime = 0;
  uint32_t now = uptime();

  uint8_t ch = 0xff; // Default value like NEMU
  if (now - lasttime > 60 * 1000) {
    // 1 minute
    eprintf(ANSI_COLOR_RED "uart16550: now = %ds\n" ANSI_COLOR_RESET, now / 1000);
    lasttime = now;
  }
  
  // Return data from RX FIFO if available
  if (rx_f != rx_r) {
    ch = uart16550_rx_dequeue();
  }
  
  return ch;
}

// Legacy getc function for compatibility
void uart16550_getc_legacy(uint8_t *ch) {
  static uint32_t lasttime = 0;
  uint32_t now = uptime();

  *ch = 0xff; // Default value
  if (now - lasttime > 60 * 1000) {
    // 1 minute
    eprintf(ANSI_COLOR_RED "uart16550: now = %ds\n" ANSI_COLOR_RESET, now / 1000);
    lasttime = now;
  }
  
  // Return data from RX FIFO if available
  if (rx_f != rx_r) {
    *ch = uart16550_rx_dequeue();
  }
}

// Put character to TX FIFO (simulated output)
void uart16550_putc(uint8_t ch) {
  uart16550_tx_enqueue(ch);
  // In real implementation, this would be sent to stdout
  printf("%c", ch);
  fflush(stdout);
}

// Read 16550 register
uint8_t uart16550_read_reg(uint8_t offset) {
  switch (offset) {
    case RBR_THR_OFFSET:
      return uart16550_rx_dequeue(); // RBR (read)
    case IER_OFFSET:
      return IER;
    case IIR_FCR_OFFSET:
      return IIR; // IIR (read)
    case LCR_OFFSET:
      return LCR;
    case MCR_OFFSET:
      return MCR;
    case LSR_OFFSET:
      return uart16550_get_lsr(); // LSR (read)
    case MSR_OFFSET:
      return MSR; // MSR (read)
    case SCR_OFFSET:
      return SCR;
    default:
      return 0xff;
  }
}

// Write 16550 register
void uart16550_write_reg(uint8_t offset, uint8_t data) {
  switch (offset) {
    case RBR_THR_OFFSET:
      uart16550_putc(data); // THR (write)
      break;
    case IER_OFFSET:
      IER = data;
      break;
    case IIR_FCR_OFFSET:
      FCR = data; // FCR (write)
      break;
    case LCR_OFFSET:
      LCR = data;
      break;
    case MCR_OFFSET:
      MCR = data;
      break;
    case MSR_OFFSET:
      // MSR is read-only
      break;
    case SCR_OFFSET:
      SCR = data;
      break;
    default:
      break;
  }
}

// Preset input for testing (like NEMU's preset_input)
static void uart16550_preset_input() {
  char debian_cmd[128] = "root\n";
  char busybox_cmd[128] =
      "ls\n"
      "echo 123\n"
      "cd /root/benchmark\n"
      "ls\n"
      "./stream\n"
      "ls\n"
      "cd /root/redis\n"
      "ls\n"
      "ifconfig -a\n"
      "./redis-server\n";
  
  char *buf = debian_cmd;
  int i;
  for (i = 0; i < strlen(buf); i++) {
    uart16550_rx_enqueue(buf[i]);
  }
}

// Initialize 16550 UART
void init_uart16550(void) {
  // Initialize FIFOs
  memset(rx_fifo, 0, sizeof(rx_fifo));
  memset(tx_fifo, 0, sizeof(tx_fifo));
  rx_f = rx_r = 0;
  tx_f = tx_r = 0;
  
  // Initialize registers
  IER = 0;
  IIR = 1; // No interrupt pending
  FCR = 0;
  LCR = 0;
  MCR = 0;
  MSR = 0;
  SCR = 0;
  
  // Preset input
  uart16550_preset_input();
}

// Cleanup 16550 UART
void finish_uart16550(void) {
  memset(rx_fifo, 0, sizeof(rx_fifo));
  memset(tx_fifo, 0, sizeof(tx_fifo));
  rx_f = rx_r = 0;
  tx_f = tx_r = 0;
}

// Get FIFO status for debugging
void uart16550_get_fifo_status(int *rx_count, int *tx_count) {
  *rx_count = (rx_r - rx_f + RX_FIFO_SIZE) % RX_FIFO_SIZE;
  *tx_count = (tx_r - tx_f + TX_FIFO_SIZE) % TX_FIFO_SIZE;
} 
