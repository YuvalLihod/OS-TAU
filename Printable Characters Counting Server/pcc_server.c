#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define BUF_SIZE 1024

uint16_t pcc_total[95] = {0}; //printable char is a byte b whose value is 32<=b<=126
int connfd = -1; //Global
int can_accept = 1; //Global

void print_and_close(){
    for(int i=0; i<95; i++){
        printf("char '%c' : %hu times\n", (i+32), pcc_total[i]);
    }
    exit(0);
}

void my_sigint_handler(int signum){
    if(connfd == -1){ //no client is being processed
        print_and_close();
    }
    //a client is being processed -> complete and don't accept another connections
    can_accept = 0;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in);
    char data_buff[BUF_SIZE];
    int listenfd, bytes_sent, bytes_left, tmp;
    uint16_t curr_pcc[95] = {0}; //printable char is a byte b whose value is 32<=b<=126
    uint16_t N;
    uint16_t C = 0;
    int optval = 1;

    if (argc != 2){
        perror("Incorrect number of arguments is passed");
        exit(1);
    }
    // SIGINT handler
    struct sigaction newAction = {.sa_handler = my_sigint_handler,
                                  .sa_flags = SA_RESTART};
    if(sigaction(SIGINT, &newAction, NULL) == -1){
        perror("Signal handle registration failed");
        exit(1);
    }

    memset(&serv_addr, 0, addrsize);
    
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error : Could not create socket");
        exit(1);
    }
    if(-1 == setsockopt(listenfd,SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))){
        perror("Error : setsockopt Failed");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons((uint16_t)atoi(argv[1])); //atoi retuns 0 on failure,do i need to check?
    if (0 != bind(listenfd, (struct sockaddr *)&serv_addr, addrsize)) {
        perror("Error : Bind Failed");
        exit(1);
    }
    
    if (0 != listen(listenfd, 10)) {
        perror("Error : Listen Failed");
        exit(1);
    }

    while (can_accept){
        connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0) {
            perror("Error : Accept Failed");
            exit(1);
        }
        C = 0;
        // Receive the number N (2 bytes) from client
        bytes_left = 2;
        bytes_sent = 0;
        while(bytes_left > 0){
            tmp = recv(connfd, (char*)&N +bytes_sent, bytes_left, 0);
            if((tmp == -1 && (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)) || tmp == 0){ //Non terminating error occurred
                perror("Error: recv from client failed. Server keep running. errno");
                close(connfd);
                connfd = -1;
                break;
            }else if (tmp == -1){ //terminating error occurred
                perror("recv");
                close(connfd);
                exit(1);  
            }else{
                bytes_left -= tmp;
                bytes_sent += tmp;   
            } 
        }
        if (connfd == -1){ //Non terminating error occurred - move to next client
            continue;
        }
        N = ntohs(N);

        // Receive N bytes from client
        memset(curr_pcc, 0 , sizeof(curr_pcc));
        bytes_left = N;
        while(bytes_left > 0){
            tmp = recv(connfd, data_buff, sizeof(data_buff),0);
            if((tmp == -1 && (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)) || tmp == 0){ //Non terminating error occurred
                perror("Error: recv from client failed. Server keep running. errno");
                close(connfd);
                connfd = -1;
                break;
            }else if (tmp == -1){ //terminating error occurred
                perror("recv");
                close(connfd);
                exit(1);  
            }else{
                for(int i=0; i<tmp ; i++){
                    if(32 <=data_buff[i] && data_buff[i] <= 126 ){
                        C++;
                        curr_pcc[(int)(data_buff[i]) -32]++;
                    }
                }
                bytes_left -= tmp;
            }
        }
        if (connfd == -1){ //Non terminating error occurred - move to next client
            continue;
        }

        // Send C to client
        C = htons(C);
        bytes_left = 2;
        bytes_sent = 0;
        while(bytes_left > 0){
            tmp = send(connfd, (char*)&C +bytes_sent, bytes_left, 0);
            if((tmp == -1 && (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)) || tmp == 0){ //Non terminating error occurred
                perror("Error: send to client failed. Server keep running. errno");
                close(connfd);
                connfd = -1;
                break;
            }else if (tmp == -1){ //terminating error occurred
                perror("send");
                close(connfd);
                exit(1);  
            }else{
                bytes_left -= tmp;
                bytes_sent += tmp;   
            } 
        }
        if (connfd == -1){ //Non terminating error occurred - move to next client
            continue;
        }

        // Update pcc_global statistics
        for(int i=0 ; i<95; i++){
            pcc_total[i] += curr_pcc[i];
        }

        close(connfd);
        connfd = -1;
    }

    //get here only when SIGINT is received while processing client
    close(listenfd);
    print_and_close();
}