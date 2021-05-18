//Based on example from https://forums.xilinx.com/t5/Embedded-Linux/Custom-Hardware-with-UIO/m-p/805185#M22515
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>


#define TAIGA_PR_QUEUE_BASEADDR             0x43C00000
#define TAIGA_PR_QUEUE_MAP_SIZE             0x10000
#define TAIGA_PR_QUEUE_PEEK_OFFSET          0x4
#define TAIGA_PR_QUEUE_POP_OFFSET           0x8
#define TAIGA_PR_QUEUE_REQUEST_PENDING_OFFSET   0xC

#define GRID_SLOT_MASK                      0x0000001F
#define GRID_SLOT_SHIFT                     0
#define OU_ID_MASK                          0x0000001F
#define OU_ID_SHIFT                         5

// #define CUSTOM_IP_S00_AXI_SLV_REG0_OFFSET   0
// #define CUSTOM_IP_S00_AXI_SLV_REG1_OFFSET   4
// #define CUSTOM_IP_S00_AXI_SLV_REG2_OFFSET   8
// #define CUSTOM_IP_S00_AXI_SLV_REG3_OFFSET   12

int main(void)
{
    int fd = open("/dev/uio0", O_RDWR);
    void *ptr;

    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    ptr = mmap(NULL, TAIGA_PR_QUEUE_MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    while (1) {
        uint32_t info = 1; /* Enable Interrupt */
        uint32_t pr_request_pending;
        uint32_t pr_request;
        uint32_t pr_request_pop;
        uint32_t grid_slot;
        uint32_t ou_id;

        ssize_t nb = write(fd, &info, sizeof(info));
        if (nb < sizeof(info)) {
            perror("write");
            close(fd);
            exit(EXIT_FAILURE);
        }

        struct pollfd fds = {
            .fd = fd,
            .events = POLLIN,
        };

        int ret = poll(&fds, 1, -1);
        if (ret >= 1) {
            nb = read(fd, &info, sizeof(info));
            if (nb == sizeof(info)) {
                /* Do something in response to the interrupt. */
                printf("Interrupt #%u!\n\r", info);

                printf("Checking PR Request Pending\n\r");
                pr_request_pending = *((unsigned *)(ptr + TAIGA_PR_QUEUE_REQUEST_PENDING_OFFSET));
                printf("PR Request Pending: %u\n\r", pr_request_pending);

                printf("Peeking PR Request\n\r");
                pr_request = *((unsigned *)(ptr + TAIGA_PR_QUEUE_PEEK_OFFSET));
                grid_slot = (pr_request >> GRID_SLOT_SHIFT) & GRID_SLOT_MASK;
                ou_id = (pr_request >> OU_ID_SHIFT) & OU_ID_MASK;
                printf("Request: OU %u in Slot %u\n\r", ou_id, grid_slot);

                printf("Popping PR Request\n\r");
                pr_request_pop = *((unsigned *)(ptr + TAIGA_PR_QUEUE_POP_OFFSET));
                if(pr_request_pop == pr_request){
                    printf("Popped PR request matches peeked PR request!\n\r");
                }else{
                    printf("Popped and Peeked PR requests do not match!\n\r");
                }
            }
        } else {
            perror("poll()");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    close(fd);
    exit(EXIT_SUCCESS);
}