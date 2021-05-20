#include "board_support.h"
#include <stdio.h>
#include "rca.h"
#include "pr.h"




int main(void) {
    printf("Hello World!\n\r");
    printf("Testing PR Queue\n\r");

    for(int i = 0; i < NUM_OUS; i++){
        printf("Putting OU %u in slot %u\n\r", i ,i);
        send_pr_request(i, i);
    }   

    return 0;
}