#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define BUF_SIZE 1024


int main(int argc, char *argv[]) {
    int fd, bytes_sent, bytes_left, bytes_read, tmp;
    int sockfd = -1;
    uint16_t printable_count;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE];

    if (argc != 4){
        perror("Incorrect number of arguments is passed");
        exit(1);
    }

    fd = open(argv[3], O_RDONLY);
    if(fd<0){
        perror("Failed to open file");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error : Could not create socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((uint16_t)atoi(argv[2])); // Note: htons for endiannes
    //atoi retuns 0 on failure,do i need to check?
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) != 1) {
        perror("inet_pton");
        exit(1);
    }
    
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error : Connect Failed");
        exit(1);
    }

    // Determine file size
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        perror("lseek");
        exit(1);
    }
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        exit(1);
    }

    // Send file size to server
    uint16_t file_size_n = htons((uint16_t)file_size);
    bytes_left = 2;
    bytes_sent = 0;
    while(bytes_left > 0){
        tmp = send(sockfd, (char *)&file_size_n +bytes_sent , bytes_left, 0); //(char*) so +X move X bytes
        if(tmp == -1){
            perror("send");
            exit(1);  
        }
        bytes_left -= tmp;
        bytes_sent += tmp;
    }


    // Send file content to server
    bytes_left = file_size;
    bytes_sent = 0;
    while(bytes_left > 0){
        bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read == -1){
            perror("read");
            exit(1);
        }
        tmp = send(sockfd, buffer, bytes_read, 0);
        if (tmp == -1) {
            perror("send");
            exit(1);
        }
        bytes_left -= tmp;
        bytes_sent += tmp;
        if (lseek(fd, bytes_sent, SEEK_SET) == -1) {
            perror("lseek");
            exit(1);
        }
    }

    // Receive the count of printable characters from the server
    bytes_left = 2;
    bytes_sent = 0;
    while(bytes_left > 0){
        tmp = recv(sockfd, (char *)&printable_count +bytes_sent, bytes_left, 0); //(char*) so +X move X bytes
        if(tmp == -1){
            perror("recv");
            exit(1);  
        }
        bytes_left -= tmp;
        bytes_sent += tmp;
    }

    printable_count = ntohs(printable_count);
    printf("# of printable characters: %hu\n", printable_count);

    close(fd);
    close(sockfd);
    exit(0);
}