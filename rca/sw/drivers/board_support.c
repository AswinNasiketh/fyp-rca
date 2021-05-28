#include "board_support.h"
#include <stdio.h>
#include <stdint.h>

//UART regs
#define TX_BUFFER_EMPTY 0x00000020
#define RX_HAS_DATA 	0x00000001
#define LINE_STATUS_REG_ADDR	(UART_RX_TX_REG + 0x14)

#define RNG_SEED		1234

////////////////////////////////////////////////////
//putc and getc support for printf
int uart_putc(char c, FILE *file) {
	(void) file;
	//Ensure space in buffer
	while (!((*(unsigned volatile char *)LINE_STATUS_REG_ADDR) & TX_BUFFER_EMPTY));
	*(unsigned volatile char*)UART_RX_TX_REG = (unsigned volatile char) c;
	return c;  
}

int uart_getc(FILE *file) {
	(void) file;
	//Wait for character
	while (!((*(unsigned volatile char *)LINE_STATUS_REG_ADDR) & RX_HAS_DATA));
	return *((unsigned volatile char*)UART_RX_TX_REG);
}

static FILE __stdio = FDEV_SETUP_STREAM(uart_putc, uart_getc, NULL, _FDEV_SETUP_RW);
FILE *const __iob[3] = {&__stdio, &__stdio, &__stdio};

void _ATTRIBUTE ((__noreturn__)) _exit (int status) {
	VERILATOR_EXIT_SUCCESS;
	while(1);
}

////////////////////////////////////////////////////
//Profiling Support
unsigned long long _start_time, _end_time, _user_time;
unsigned long long _start_instruction_count, _end_instruction_count, _user_instruction_count;
unsigned long long _scaled_IPC;

//Read cycle CSR
unsigned long long _read_cycle()
{
	unsigned long long result;
	unsigned long lower;
	unsigned long upper1;
	unsigned long upper2;
	
	asm volatile (
		"repeat_cycle_%=: csrr %0, cycleh;\n"
		"        csrr %1, cycle;\n"     
		"        csrr %2, cycleh;\n"
		"        bne %0, %2, repeat_cycle_%=;\n" 
		: "=r" (upper1),"=r" (lower),"=r" (upper2)    // Outputs   : temp variable for load result
		: 
		: 
	);
	*(unsigned long *)(&result) = lower;
	*((unsigned long *)(&result)+1) = upper1;

	return result;
}

//Read instruction count CSR
unsigned long long  _read_inst()
{
	unsigned long long  result;
	unsigned long lower;
	unsigned long upper1;
	unsigned long upper2;
	
	asm volatile (
		"repeat_inst_%=: csrr %0, instreth;\n"
		"        csrr %1, instret;\n"     
		"        csrr %2, instreth;\n"
		"        bne %0, %2, repeat_inst_%=;\n" 
		: "=r" (upper1),"=r" (lower),"=r" (upper2)    // Outputs   : temp variable for load result
		: 
		: 
	);
	*(unsigned long *)(&result) = lower;
	*((unsigned long *)(&result)+1) = upper1;

	return result;
}

void platform_init (uint32_t trap_handler_addr) {
  asm volatile("csrrw x0, mtvec, %0"
	:
	:"r"(trap_handler_addr)
	:
  );

  srand(RNG_SEED);
  init_grid();
}

void start_profiling ()  {
	_start_time = _read_cycle();
	_start_instruction_count = _read_inst();
	VERILATOR_START_PROFILING;
}

void end_profiling ()  {
	VERILATOR_STOP_PROFILING;
	_end_time = _read_cycle();
	_end_instruction_count = _read_inst();
    
	_user_time = _end_time - _start_time;
	_user_instruction_count = _end_instruction_count - _start_instruction_count;
	_scaled_IPC = (_user_instruction_count*1000000)/_user_time;
    
	printf("Start time: %lld\r\n", _start_time);
	printf("End time: %lld\r\n", _end_time);
	printf("User time: %lld\r\n", _user_time);
	printf("start inst: %lld\r\n", _start_instruction_count);
	printf("End inst: %lld\r\n", _end_instruction_count);
	printf("User inst: %lld\r\n", _user_instruction_count);
	printf("IPCx1M: %lld\r\n", _scaled_IPC);
}

//Trap Handling

void handle_trap(trapframe_t* tf){
    if(tf->cause == MCAUSE_PROFILER_EX){
		printf("Profiler exception!\n\r");
		handle_profiler_exception();
	}else{
		printf("Unhandled exception!\n\r");
		while(1);
	}
}
