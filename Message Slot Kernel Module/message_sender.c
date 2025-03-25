#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "message_slot.h"

int main(int argc, char *argv[]){
    int fd, ret_val, msg_len;
    unsigned int id;
    if (argc != 4){
        perror("Incorrect number of arguments is passed\n");
        exit(1);
    }
    fd = open( argv[1], O_WRONLY);
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
    msg_len = strlen(argv[3]); //Doesn't include terminating null character
    ret_val = write( fd, argv[3] , msg_len);
    if(ret_val != msg_len){
        perror("Failed to write message to the message slot file\n");
        exit(1); 
    }
    close(fd);
    exit(0);
}