#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "stdint.h"
#include "stdbool.h"
#include "pcap.h"
#include "time.h"

#define     XDFG_LEN                    0x200
#define     XDCFG_OFFSET                0xF8007000

#define     XDCFG_CTRL_OFFSET           0x00000000
#define     XDCFG_CTRL_PCAP_PR_MASK     0x08000000 
#define     XDCFG_CTRL_PCAP_MODE_MASK   0x04000000
#define     XDCFG_CTRL_PCAP_RATE_EN_MASK 0x02000000

#define     XDCFG_INT_STATUS_OFFSET     0x0000000C
#define     XDCFG_INT_STATUS_DMA_DONE_MASK 0x00002000
#define     XDCFG_INT_STATUS_AXI_WERR_MASK 0x00400000
#define     XDCFG_INT_STATUS_AXI_RTO_MASK 0x00200000
#define     XDCFG_INT_STATUS_AXI_RERR_MASK 0x00100000
#define     XDCFG_INT_STATUS_RX_FIFO_OV_MASK 0x00040000
#define     XDCFG_INT_STATUS_RX_FIFO_OV_MASK 0x00040000
#define     XDCFG_INT_STATUS_DMA_CMD_ERR_MASK 0x00008000
#define     XDCFG_INT_STATUS_DMA_Q_OV_MASK 0x00004000
#define     XDCFG_INT_STATUS_P2D_LEN_ERR_MASK 0x00000800
#define     XDCFG_INT_STATUS_PCFG_HMAC_ERR_MASK 0x00000040
#define     XDCFG_INT_STATUS_PCFG_DONE_MASK 0x00000004

#define     XDCFG_STATUS_OFFSET         0x00000014
#define     XDCFG_STATUS_PCFG_INIT_MASK 0x00000010
#define     XDCFG_STATUS_DMA_CMD_Q_F_MASK 0x80000000

#define     XDCFG_MCTRL_OFFSET         	0x00000080
#define     XDCFG_MCTRL_PCAP_LPBK_MASK  0x00000010

#define     XDCFG_DMA_SRC_ADDR_OFFSET   0x00000018
#define     XDCFG_DMA_DEST_ADDR_OFFSET  0x0000001C
#define     XDCFG_DMA_SRC_LEN_OFFSET    0x00000020
#define     XDCFG_DMA_DEST_LEN_OFFSET   0x00000024

volatile uint32_t read_reg(volatile uint32_t* mem_ptr, uint32_t page_offset, uint32_t reg_offset){
//    printf("Requested Reg Addr: %lx \n", mem_ptr + page_offset + reg_offset);
    mem_ptr += page_offset;
    return mem_ptr[reg_offset >> 2];
}

void write_reg(volatile uint32_t* mem_ptr, uint32_t page_offset, uint32_t reg_offset, uint32_t write_val){
    mem_ptr += page_offset;
    mem_ptr[reg_offset >> 2] = write_val;
}

void enable_PCAP(volatile uint32_t* mem_ptr, uint32_t page_offset){
    uint32_t reg_ctrl = read_reg(mem_ptr, page_offset, XDCFG_CTRL_OFFSET);

    uint32_t new_reg_ctrl = reg_ctrl | XDCFG_CTRL_PCAP_PR_MASK | XDCFG_CTRL_PCAP_MODE_MASK;

    write_reg(mem_ptr, page_offset, XDCFG_CTRL_OFFSET, new_reg_ctrl);
}

void clear_interrupts(volatile uint32_t* mem_ptr, uint32_t page_offset){
    write_reg(mem_ptr, page_offset, XDCFG_INT_STATUS_OFFSET, 0xFFFFFFFF);
}

bool check_pl_ready(volatile uint32_t* mem_ptr, uint32_t page_offset){
    uint32_t reg_status = read_reg(mem_ptr, page_offset, XDCFG_STATUS_OFFSET);
    // printf("STATUS REG: %x \n", reg_status);
    if((reg_status & XDCFG_STATUS_PCFG_INIT_MASK) == XDCFG_STATUS_PCFG_INIT_MASK){
        return true;
    }
    return false;
}

bool check_dma_cmd_q_full(volatile uint32_t* mem_ptr, uint32_t page_offset){
    uint32_t reg_status = read_reg(mem_ptr, page_offset, XDCFG_STATUS_OFFSET);
    return (reg_status & XDCFG_STATUS_DMA_CMD_Q_F_MASK) == XDCFG_STATUS_DMA_CMD_Q_F_MASK; 
}

void disable_pcap_lpbk(volatile uint32_t* mem_ptr, uint32_t page_offset){
    uint32_t reg_mctrl = read_reg(mem_ptr, page_offset, XDCFG_MCTRL_OFFSET);

    uint32_t new_reg_mctrl = reg_mctrl & (~XDCFG_MCTRL_PCAP_LPBK_MASK); 

    write_reg(mem_ptr, page_offset, XDCFG_MCTRL_OFFSET, new_reg_mctrl);
}

void disable_2x_clk_div(volatile uint32_t* mem_ptr, uint32_t page_offset){
    uint32_t reg_ctrl = read_reg(mem_ptr, page_offset, XDCFG_CTRL_OFFSET);

    uint32_t new_reg_ctrl = reg_ctrl & (~XDCFG_CTRL_PCAP_RATE_EN_MASK); 

    write_reg(mem_ptr, page_offset, XDCFG_CTRL_OFFSET, new_reg_ctrl);
}

uint32_t read_file_into_buf(unsigned char* buf_ptr, char* file_name){
    FILE* fileptr = fopen(file_name, "rb");
    if(fileptr == NULL){
        printf("Error opening file %s\n", file_name);
    }
    //find file length
    fseek(fileptr, 0, SEEK_END);
    uint32_t filelen = ftell(fileptr);
    rewind(fileptr); 
    //read file
    fread(buf_ptr, 1, filelen, fileptr);

    fclose(fileptr);
    // for(int i = 0; i < 20; i++){
    //     printf("%x\n", (buf_ptr)[i]);
    // }
    return (filelen >> 2);
}

void queue_dma_transfer(volatile uint32_t* mem_ptr, uint32_t page_offset, unsigned char* bs_ptr, uint32_t bs_len){
   write_reg(mem_ptr, page_offset, XDCFG_DMA_SRC_ADDR_OFFSET, (uint32_t) bs_ptr);
   write_reg(mem_ptr, page_offset, XDCFG_DMA_DEST_ADDR_OFFSET, 0xFFFFFFFF);
   write_reg(mem_ptr, page_offset, XDCFG_DMA_SRC_LEN_OFFSET, bs_len);
   write_reg(mem_ptr, page_offset, XDCFG_DMA_DEST_LEN_OFFSET, bs_len);
}

bool check_for_errors(volatile uint32_t* mem_ptr, uint32_t page_offset){
    uint32_t reg_int_sts = read_reg(mem_ptr, page_offset, XDCFG_INT_STATUS_OFFSET);
    uint32_t err = reg_int_sts & 
                   (XDCFG_INT_STATUS_AXI_WERR_MASK |
                    XDCFG_INT_STATUS_AXI_RTO_MASK |
                    XDCFG_INT_STATUS_AXI_RERR_MASK |
                    XDCFG_INT_STATUS_RX_FIFO_OV_MASK |
                    XDCFG_INT_STATUS_DMA_CMD_ERR_MASK |
                    XDCFG_INT_STATUS_DMA_Q_OV_MASK |
                    XDCFG_INT_STATUS_P2D_LEN_ERR_MASK |
                    XDCFG_INT_STATUS_PCFG_HMAC_ERR_MASK);
    if(err != 0){
        printf("Error after DMA complete! %x \n", err);
        return true;
    }

    return false;    
}

void poll_dma_transfer_complete(volatile uint32_t* mem_ptr, uint32_t page_offset){
    uint32_t reg_int_sts = read_reg(mem_ptr, page_offset, XDCFG_INT_STATUS_OFFSET);
    while((reg_int_sts & XDCFG_INT_STATUS_DMA_DONE_MASK) != XDCFG_INT_STATUS_DMA_DONE_MASK){
         reg_int_sts = read_reg(mem_ptr, page_offset, XDCFG_INT_STATUS_OFFSET);
         if(check_for_errors(mem_ptr, page_offset)) break;
    }
}


void poll_pl_cfg_done(volatile uint32_t* mem_ptr, uint32_t page_offset){
    uint32_t reg_int_sts = read_reg(mem_ptr, page_offset, XDCFG_INT_STATUS_OFFSET);
    while((reg_int_sts & XDCFG_INT_STATUS_PCFG_DONE_MASK) != XDCFG_INT_STATUS_PCFG_DONE_MASK){
        reg_int_sts = read_reg(mem_ptr, page_offset, XDCFG_INT_STATUS_OFFSET);
    }  
}

unsigned char* init_dma_buf(){
    //allocate DMA buffer
    char command[200] = "insmod u-dma-buf.ko udmabuf0=\0";
    sprintf(command, "%s%u", command, UDMABUF_SIZE);
    system(command);

    //get DMA buffer physical address
    unsigned char* phys_addr;
    int dma_phys_addr_fd;
    unsigned char  attr[1024];
    if ((dma_phys_addr_fd  = open("/sys/class/u-dma-buf/udmabuf0/phys_addr", O_RDONLY)) != -1) {
        read(dma_phys_addr_fd, attr, 1024);
        sscanf(attr, "%x", &phys_addr);
        close(dma_phys_addr_fd);
    }
    printf("DMA BUF Phys addr: %x\n", phys_addr);

    return phys_addr;
}

//mmap variables for XDCFG
size_t page_size;
off_t page_base;
off_t page_offset;

unsigned char*  dma_buf_phys_addr;
volatile unsigned char* dma_mmap;
volatile uint32_t* mem_mmap;

//variables for bitstream data
char pb_paths[NUM_GRID_SLOTS][MAX_PATH_LEN]; 
char ou_file_names[NUM_OUS][20];
buf_entry_t buf_entries[NUM_OUS][NUM_GRID_SLOTS];
char bs_buf[BS_BUF_SIZE];

void populate_paths(){
    uint32_t row, col;
    for(int i = 0; i < NUM_GRID_SLOTS; i++){
        //Calculate grid row and column
        row = i/NUM_GRID_COLS;
        col = i % NUM_GRID_COLS;

        //Form path string and store to array
        sprintf(pb_paths[i], "rps/rp_%u_%u/", row, col);
    }
}

void populate_ou_names(){
    sprintf(ou_file_names[0], "greybox.bit.bin");
    sprintf(ou_file_names[1], "and.bit.bin"); //never going to be used
    sprintf(ou_file_names[2], "add.bit.bin");
    sprintf(ou_file_names[3], "and.bit.bin");
    sprintf(ou_file_names[4], "auipc.bit.bin");
    sprintf(ou_file_names[5], "lb.bit.bin");
    sprintf(ou_file_names[6], "lbu.bit.bin");
    sprintf(ou_file_names[7], "lh.bit.bin");
    sprintf(ou_file_names[8], "lhu.bit.bin");
    sprintf(ou_file_names[9], "lui.bit.bin");
    sprintf(ou_file_names[10], "lw.bit.bin");
    sprintf(ou_file_names[11], "or.bit.bin");
    sprintf(ou_file_names[12], "sb.bit.bin");
    sprintf(ou_file_names[13], "sh.bit.bin");
    sprintf(ou_file_names[14], "sll.bit.bin");
    sprintf(ou_file_names[15], "slt.bit.bin");
    sprintf(ou_file_names[16], "sltu.bit.bin");
    sprintf(ou_file_names[17], "sra.bit.bin");
    sprintf(ou_file_names[18], "srl.bit.bin");
    sprintf(ou_file_names[19], "sub.bit.bin");
    sprintf(ou_file_names[20], "sw.bit.bin");
    sprintf(ou_file_names[21], "xor.bit.bin");
}

void load_pbs_into_buf(){
    populate_paths();
    populate_ou_names();

    uint32_t buf_offset = 0;

    for(int i = 0; i < NUM_GRID_SLOTS; i++){
        for(int j = 0; j < NUM_OUS; j++){
            char path[200] = "";
            strcat(path, pb_paths[i]);
            strcat(path, ou_file_names[j]);

            uint32_t file_len_words = read_file_into_buf(bs_buf+buf_offset, path);

            buf_entries[j][i].bs_buf_offset = buf_offset;
            buf_entries[j][i].bs_length_words = file_len_words;

            buf_offset += file_len_words << 2;
        }
    }
    printf("Done loading bitstreams into memory\n\r");
}


bool init_pcap_driver(){
    page_size = sysconf(_SC_PAGE_SIZE);
    page_base = (XDCFG_OFFSET / page_size) * page_size;
    page_offset = XDCFG_OFFSET - page_base;

    //Set up DMA Buf
    dma_buf_phys_addr = init_dma_buf();

    //Get DMA Buf mmap
    int dma_buf_fd = open("/dev/udmabuf0", O_RDWR | O_SYNC);
    dma_mmap = mmap(NULL, UDMABUF_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, dma_buf_fd, 0); 

    if (dma_mmap == MAP_FAILED) {
        printf("Can't map DMA Buffer\n");
        return false;
    }

    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    mem_mmap = mmap(NULL, page_offset + XDFG_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, page_base);

    if (mem_mmap == MAP_FAILED) {
        printf("Can't map memory\n");
        return false;
    }

    load_pbs_into_buf();

    return true;    
}

// bool write_partial_bitstream(char* bitstream_path){
//     enable_PCAP(mem_mmap, page_offset);

//     clear_interrupts(mem_mmap, page_offset);

//     bool is_pl_ready = check_pl_ready(mem_mmap, page_offset);
//     if(!is_pl_ready){
//         printf("Error Writing Parital Bitstream: PL Not Ready!\n");
//         return false;
//     }

//     bool dma_q_full = check_dma_cmd_q_full(mem_mmap, page_offset);
//     if(dma_q_full){
//         printf("Error Writing Parital Bitstream: DMA Command Queue full!\n");
//         return false;
//     }

//     disable_pcap_lpbk(mem_mmap, page_offset);

//     disable_2x_clk_div(mem_mmap, page_offset);

//     uint32_t file_len = read_file_into_buf(dma_mmap, bitstream_path);

//     queue_dma_transfer(mem_mmap, page_offset, dma_buf_phys_addr, file_len);

//     poll_dma_transfer_complete(mem_mmap, page_offset);

//     bool err = check_for_errors(mem_mmap, page_offset);
//     if(err){
//         return false;
//     }

//     poll_pl_cfg_done(mem_mmap, page_offset);
//     printf("PL CFG Done!\n");
//     return true;
// }

bool write_partial_bitstream(uint32_t grid_slot, uint32_t ou){
    // clock_t t;
    // t = clock();
    enable_PCAP(mem_mmap, page_offset);

    clear_interrupts(mem_mmap, page_offset);

    bool is_pl_ready = check_pl_ready(mem_mmap, page_offset);
    if(!is_pl_ready){
        printf("Error Writing Parital Bitstream: PL Not Ready!\n");
        return false;
    }

    bool dma_q_full = check_dma_cmd_q_full(mem_mmap, page_offset);
    if(dma_q_full){
        printf("Error Writing Parital Bitstream: DMA Command Queue full!\n");
        return false;
    }

    disable_pcap_lpbk(mem_mmap, page_offset);

    disable_2x_clk_div(mem_mmap, page_offset);

    buf_entry_t slot_ou_entry = buf_entries[ou][grid_slot];
    // printf("Writing bitstream for OU %u in Slot %u\n", ou, grid_slot);
    memcpy(dma_mmap, bs_buf+slot_ou_entry.bs_buf_offset, slot_ou_entry.bs_length_words * 4 * sizeof(unsigned char));
    queue_dma_transfer(mem_mmap, page_offset, dma_buf_phys_addr, slot_ou_entry.bs_length_words);

    poll_dma_transfer_complete(mem_mmap, page_offset);

    bool err = check_for_errors(mem_mmap, page_offset);
    if(err){
        return false;
    }

    poll_pl_cfg_done(mem_mmap, page_offset);
    // t = clock() - t;
    // double time_taken = ((double)t)/CLOCKS_PER_SEC;
    // printf("PL CFG Done! Took %.15f seconds!\n", time_taken);
    return true;
}