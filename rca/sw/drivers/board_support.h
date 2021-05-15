#ifndef BOARD_SUPPORT_H
#define BOARD_SUPPORT_H
#include <stdio.h>

#define CPU_MHZ 1

//Base address for a 16550 UART
#define UART_BASE_ADDR 0x60000000
#define UART_RX_TX_REG (UART_BASE_ADDR + 0x1000)

//64-bit variables for cycle and instruction counts
extern unsigned long long _start_time, _end_time, _user_time;
extern unsigned long long _start_instruction_count, _end_instruction_count, _user_instruction_count;
extern unsigned long long _scaled_IPC;

//Custom NOPs for Verilator logging and operation
#define  VERILATOR_START_PROFILING __asm__ volatile ("addi x0, x0, 0xC" : : : "memory")
#define  VERILATOR_STOP_PROFILING __asm__ volatile ("addi x0, x0, 0xD" : : : "memory")
#define  VERILATOR_EXIT_SUCCESS __asm__ volatile ("addi x0, x0, 0xA" : : : "memory")
#define  VERILATOR_EXIT_ERROR __asm__ volatile ("addi x0, x0, 0xF" : : : "memory")

//External Functions
int uart_putc(char c, FILE *file);
int uart_getc(FILE *file);

void platform_init ();
void start_profiling ();
void end_profiling ();

#endif