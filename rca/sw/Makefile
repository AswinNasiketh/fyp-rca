#set target binary name, memory size and CFLAGS
#the makefile will build all c files in the src directory

#Output binary name
TARGET = hello_world

#Platform Details
BASE_MEMORY_ADDR = 0x80000000
UART_BASE_ADDR = 0x60000000
MEM_SIZE_KB = 32
STACK_SIZE_KB = 2

#Sizes coverted into bytes
MEM_SIZE_B = $$(($(MEM_SIZE_KB)*1024))
STACK_SIZE_B = $$(($(STACK_SIZE_KB)*1024))

BOARD_CFLAGS = -DUART_BASE_ADDR=$(UART_BASE_ADDR) -Xlinker --defsym=__mem_addr=$(BASE_MEMORY_ADDR) -Xlinker --defsym=__mem_size=$(MEM_SIZE_B) -Xlinker  --defsym=__stack_size=$(STACK_SIZE_B)

#picolibc.specs includes -ffunction-sections and linker garbage collection
CFLAGS = $(BOARD_CFLAGS) --specs=picolibc.specs -march=rv32im -mabi=ilp32 -O2 -fno-inline -DDHRYSTONE_ITERATIONS=100000
LDFLAGS = -Wl,--print-memory-usage --verbose

###############################
#Don't Change past this point
###############################
CC = riscv32-unknown-elf-gcc
ELF_TO_HW_INIT = python3 $(TAIGA_DIR)/tools/taiga_binary_converter.py riscv32-unknown-elf- $(BASE_MEMORY_ADDR) $(MEM_SIZE_B)

SRC_DIR := src
SRCS := $(wildcard $(SRC_DIR)/*.c)
ASM_SRCS := $(wildcard $(SRC_DIR)/*.S)
OBJDIR := obj
DEPDIR := $(OBJDIR)/.deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

SRCS_NO_PATH := $(notdir $(SRCS))
ASM_SRCS_NO_PATH := $(notdir $(ASM_SRCS))
OBJECTS := $(SRCS_NO_PATH:%.c=$(OBJDIR)/%.o)
ASM_OBJS := $(ASM_SRCS_NO_PATH:%.S=$(OBJDIR)/%.o)

all: $(TARGET).hw_init 

#Produce ram init files
$(TARGET).hw_init : $(TARGET)
	$(ELF_TO_HW_INIT) $< $@ $<.sim_init

#Link
$(TARGET): $(OBJECTS) $(ASM_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)
	
#Compile
$(OBJDIR)/%.o : $(SRC_DIR)/%.c $(DEPDIR)/%.d | $(DEPDIR) 
	$(CC) $(DEPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/trap_entry.o : $(SRC_DIR)/trap_entry.S 
	$(CC) $(CFLAGS) -c -o $@ $<

$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(SRCS_NO_PATH:%.c=$(DEPDIR)/%.d)
# ASM_DEPS := $(SRCS_NO_PATH:%.S=$(DEPDIR)/%.d)
$(DEPFILES):
include $(wildcard $(DEPFILES))
# include $(wildcard $(ASM_DEPS))

.PHONY: clean all
clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET).*

