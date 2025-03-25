#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include "message_slot.h"

int main(int argc, char *argv[]){
    int fd, ret_val;
    unsigned int id;
    char msg[BUF_LEN];
    if (argc != 3){
        perror("Incorrect number of arguments is passed\n");
        exit(1);
    }
    fd = open( argv[1], O_RDONLY);
    if(fd<0){
        perror("Failed to open message slot device file\n");
        exit(1);
    }
    id = atoi(argv[2]);//returns 0 on failure
    ret_val = ioctl(fd, MSG_SLOT_CHANNEL, id);
    if(ret_val < 0){
        perror("Failed to set channel id\n");
        exit(1); 
    }
    
    ret_val = read(fd, msg, BUF_LEN);
    if(ret_val <= 0){
        perror("Failed to read message from message slot file\n");
        exit(1); 
    }
    close(fd);
    if(write(STDOUT_FILENO, msg, ret_val) != ret_val){
        perror("Failed to print the whole message to standard output\n");
        exit(1);
    }
    exit(0);
}