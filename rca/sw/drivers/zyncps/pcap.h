#ifndef PCAP_H
#define PCAP_H

#include "stdbool.h"
#include "stdint.h"
#include "string.h"

#define     UDMABUF_SIZE    153600 
#define     BS_BUF_SIZE     105000000    

#define MAX_PATH_LEN                        100
#define NUM_GRID_SLOTS                      30
#define NUM_GRID_COLS                       6
#define NUM_OUS                             22

bool init_pcap_driver();
// bool write_partial_bitstream(char* bitstream_path);
bool write_partial_bitstream(uint32_t grid_slot, uint32_t ou);

typedef struct{
    uint32_t bs_buf_offset;
    uint32_t bs_length_words;
}buf_entry_t;

#endif //PCAP_H