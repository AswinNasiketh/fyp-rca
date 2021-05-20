//Based on example from https://forums.xilinx.com/t5/Embedded-Linux/Custom-Hardware-with-UIO/m-p/805185#M22515
//TODO: Change to using Incomplete Requests model
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>

#define TAIGA_PR_QUEUE_BASEADDR             0x43C00000
#define TAIGA_PR_QUEUE_MAP_SIZE             0x10000
#define TAIGA_PR_QUEUE_COMPLETE_OFFSET      0x8
#define TAIGA_PR_QUEUE_POP_OFFSET           0x4
#define TAIGA_PR_QUEUE_REQUEST_PENDING_OFFSET   0xC

#define GRID_SLOT_MASK                      0x0000001F
#define GRID_SLOT_SHIFT                     0
#define OU_ID_MASK                          0x0000001F
#define OU_ID_SHIFT                         5

#define MAX_PATH_LEN                        100
#define NUM_GRID_SLOTS                      30
#define NUM_GRID_COLS                       6
#define NUM_OUS                             22

typedef enum{
    UNUSED = 0,
    PASSTHROUGH = 1,
    ADD = 2,
    AND = 3,
    AUIPC = 4,
    LB = 5,
    LBU = 6,
    LH = 7,
    LHU = 8,
    LUI = 9,
    LW = 10,
    OR = 11,
    SB = 12,
    SH = 13,
    SLL = 14,
    SLT = 15,
    SLTU = 16,
    SRA = 17,
    SRL = 18,
    SUB = 19,
    SW = 20,
    XOR = 21
}ou_t;

typedef struct{
    uint32_t grid_slot;
    ou_t ou;
} pr_request_t;


//Quick linked list queue implementation - based on https://www.geeksforgeeks.org/queue-linked-list-implementation/
typedef struct queue_item_t queue_item_t;
struct queue_item_t{
    pr_request_t request;
    struct queue_item_t* next_item;
};

typedef struct{
    queue_item_t* front;
    queue_item_t* rear;
}queue_t;

void enqueue_request(queue_t* queue, pr_request_t request){
    queue_item_t* new_item = (queue_item_t *) malloc(sizeof(queue_item_t));
    new_item->request = request;
    new_item->next_item = NULL;
    if(queue->front == NULL && queue->rear == NULL){
        queue->front = new_item;
        queue->rear = new_item;
    }else{
        queue->rear->next_item = new_item;
        queue->rear = new_item;
    }
}

pr_request_t dequeue_request(queue_t* queue){
    pr_request_t request = queue->front->request;
    queue_item_t* tmp = queue->front;
    queue->front = queue->front->next_item;

    if(queue->front == NULL){
        queue->rear = NULL;
    }
    free(tmp);
    return request;
}

uint32_t is_queue_empty(queue_t* queue){
    return (queue->front == NULL && queue->rear == NULL);
}

queue_t create_queue(){
    queue_t queue = {.front = NULL, .rear = NULL};
    return queue;
}

uint32_t check_pending_status(void* ptr){
    uint32_t pr_request_pending;
    printf("Checking PR Request Pending\n\r");
    pr_request_pending = *((unsigned *)(ptr + TAIGA_PR_QUEUE_REQUEST_PENDING_OFFSET));
    printf("PR Request Pending: %u\n\r", pr_request_pending);
    return pr_request_pending;
}

pr_request_t get_pr_request(void* ptr){
    uint32_t pr_request_raw;
    uint32_t grid_slot;
    uint32_t ou_id;

    printf("Peeking PR Request\n\r");
    pr_request_raw = *((unsigned *)(ptr + TAIGA_PR_QUEUE_POP_OFFSET));
    grid_slot = (pr_request_raw >> GRID_SLOT_SHIFT) & GRID_SLOT_MASK;
    ou_id = (pr_request_raw >> OU_ID_SHIFT) & OU_ID_MASK;
    printf("Request: OU %u in Slot %u\n\r", ou_id, grid_slot);

    pr_request_t pr_request = {.grid_slot = grid_slot, .ou = (ou_t) ou_id};
    return pr_request;
}

uint32_t pop_pr_request(void* ptr, pr_request_t pr_request){
    uint32_t pr_request_pop_raw;
    uint32_t grid_slot;
    uint32_t ou_id;

    printf("Popping PR Request\n\r");
    pr_request_pop_raw = *((unsigned *)(ptr + TAIGA_PR_QUEUE_POP_OFFSET));
    grid_slot = (pr_request_pop_raw >> GRID_SLOT_SHIFT) & GRID_SLOT_MASK;
    ou_id = (pr_request_pop_raw >> OU_ID_SHIFT) & OU_ID_MASK;

    return (grid_slot == pr_request.grid_slot && ou_id == pr_request.ou);
}


void populate_paths(char pb_path[NUM_GRID_SLOTS][MAX_PATH_LEN]){
    uint32_t row, col;
    for(int i = 0; i < NUM_GRID_SLOTS; i++){
        //Calculate grid row and column
        row = i/NUM_GRID_COLS;
        col = i % NUM_GRID_COLS;

        //Form path string and store to array
        sprintf(pb_path[i], "~/rps/rp_%u_%u/", row, col);
    }
}

void populate_ou_names(char ou_file_names[NUM_OUS][20]){
    sprintf(ou_file_names[0], "greybox.bin");
    sprintf(ou_file_names[1], "pt.bin");
    sprintf(ou_file_names[2], "add.bin");
    sprintf(ou_file_names[3], "and.bin");
    sprintf(ou_file_names[4], "auipc.bin");
    sprintf(ou_file_names[5], "lb.bin");
    sprintf(ou_file_names[6], "lbu.bin");
    sprintf(ou_file_names[7], "lh.bin");
    sprintf(ou_file_names[8], "lhu.bin");
    sprintf(ou_file_names[9], "lui.bin");
    sprintf(ou_file_names[10], "lw.bin");
    sprintf(ou_file_names[11], "or.bin");
    sprintf(ou_file_names[12], "sb.bin");
    sprintf(ou_file_names[13], "sh.bin");
    sprintf(ou_file_names[14], "sll.bin");
    sprintf(ou_file_names[15], "slt.bin");
    sprintf(ou_file_names[16], "sltu.bin");
    sprintf(ou_file_names[17], "sra.bin");
    sprintf(ou_file_names[18], "srl.bin");
    sprintf(ou_file_names[19], "sub.bin");
    sprintf(ou_file_names[20], "sw.bin");
    sprintf(ou_file_names[21], "xor.bin");
}

void do_pr_request(pr_request_t pr_request, char pb_paths[NUM_GRID_SLOTS][MAX_PATH_LEN],char ou_file_names[NUM_OUS][20]){
    char command[200] = "./fpgautil -b ";
    strcat(command, pb_paths[pr_request.grid_slot]);
    strcat(command, ou_file_names[pr_request.ou]);
    strcat(command, " -f Partial");
    // printf("%s\n\r", command);
    system(command);
}

uint32_t complete_pr_request(void* ptr){
    uint32_t pr_request_pop_raw;

    // printf("Completing PR Request\n\r");
    pr_request_pop_raw = *((unsigned volatile *)(ptr + TAIGA_PR_QUEUE_COMPLETE_OFFSET));   

    return pr_request_pop_raw; //unpredictable
}

char pb_path[NUM_GRID_SLOTS][MAX_PATH_LEN]; 
char ou_file_names[NUM_OUS][20];
queue_t pr_request_queue;
pthread_mutex_t queue_lock;
volatile uint32_t completed_requests;
pthread_mutex_t completed_requests_lock;
pthread_t request_service_thread;

void *do_pr_thread_fn(void *arg){
    

    uint32_t queue_empty = 1;
    pr_request_t pr_request;

    while(1){
        while(queue_empty == 1){
            pthread_mutex_lock(&queue_lock);
            queue_empty = is_queue_empty(&pr_request_queue);
            pthread_mutex_unlock(&queue_lock);
        }
        pthread_mutex_lock(&queue_lock);
        pr_request = dequeue_request(&pr_request_queue);
        pthread_mutex_unlock(&queue_lock);

        do_pr_request(pr_request, pb_path, ou_file_names);

        pthread_mutex_lock(&completed_requests_lock);
        completed_requests++;
        pthread_mutex_unlock(&completed_requests_lock);

        pthread_mutex_lock(&queue_lock);
        queue_empty = is_queue_empty(&pr_request_queue);
        pthread_mutex_unlock(&queue_lock);
    }

}

int main(void)
{
    populate_paths(pb_path);
    populate_ou_names(ou_file_names);

    pr_request_queue = create_queue();

    //taken from https://www.geeksforgeeks.org/mutex-lock-for-linux-thread-synchronization/
    if (pthread_mutex_init(&queue_lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    if (pthread_mutex_init(&completed_requests_lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    int fd = open("/dev/uio0", O_RDWR);
    void *ptr;

    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    ptr = mmap(NULL, TAIGA_PR_QUEUE_MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    volatile uint32_t completed_requests_lcl;
    uint32_t total_requests_completed = 0;

    int iret = pthread_create(&request_service_thread, NULL, do_pr_thread_fn, NULL);

    while (1) {
        uint32_t info = 1; /* Enable Interrupt */
        uint32_t pr_request_pending;
        pr_request_t pr_request;

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

        int ret = poll(&fds, 1, 5);
        if (ret >= 1) {
            nb = read(fd, &info, sizeof(info));
            if (nb == sizeof(info)) {
                /* Do something in response to the interrupt. */
                printf("Interrupt #%u!\n\r", info);

                pr_request_pending = check_pending_status(ptr);
                while(pr_request_pending == 1){
                    pr_request = get_pr_request(ptr);
                    pthread_mutex_lock(&queue_lock);
                    enqueue_request(&pr_request_queue, pr_request);
                    pthread_mutex_unlock(&queue_lock);
                    pr_request_pending = check_pending_status(ptr);
                }
            }
        } 
        // else {
        //     perror("poll()");
        //     close(fd);
        //     exit(EXIT_FAILURE);
        // }
        pthread_mutex_lock(&completed_requests_lock);
        completed_requests_lcl = completed_requests;
        completed_requests = 0;
        pthread_mutex_unlock(&completed_requests_lock);
        total_requests_completed += completed_requests_lcl;
        for(int i = 0; i < completed_requests_lcl; i++){
            complete_pr_request(ptr);
        }
        if(completed_requests_lcl > 0){
            printf("-----------------Completed %u Requests \n\r", completed_requests_lcl);
            printf("Total requests completed: %u \n\r", total_requests_completed);
        }        
    }

    close(fd);
    exit(EXIT_SUCCESS);
}