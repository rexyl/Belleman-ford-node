// sender
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <time.h>
#include <sys/time.h>
#include <vector>
#include <algorithm>
#include <unordered_map>

// int main(int argc, char const *argv[])
// {
//     int rev_fd;             /* receive socket */
//     int recvlen;
//     if ((rev_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
//         perror("cannot create socket\n");
//         return 0;
//     }
//     struct sockaddr_in myaddr_r;  /* our address */
//     struct sockaddr_in remaddr_r; /* remote address, which will filled by recvfrom */
//     socklen_t addrlen = sizeof(remaddr_r);        /* length of addresses */
//     memset((char *)&myaddr_r, 0, sizeof(myaddr_r));
//     myaddr_r.sin_family = AF_INET;
//     myaddr_r.sin_addr.s_addr = htonl(INADDR_ANY);
//     myaddr_r.sin_port = htons(atoi(argv[1]));
//     //myaddr_r.sin_port = htons(4119);
//     if (bind(rev_fd, (struct sockaddr *)&myaddr_r, sizeof(myaddr_r)) < 0) {
//         perror("bind failed");
//         return 0;
//     }
//     char buf[1024];
//     recvlen = recvfrom(rev_fd, buf, 1024, 0, (struct sockaddr *)&remaddr_r, &addrlen);
//     char *ipstring = inet_ntoa(remaddr_r.sin_addr);
//     int rem_p = ntohs(remaddr_r.sin_port);
//     printf("%s\n",rem_p);
//     return 0;
// }

void init_recvsock(int port){
    if ((rev_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        return 0;
    }
    
    memset((char *)&myaddr_r, 0, sizeof(myaddr_r));
    myaddr_r.sin_family = AF_INET;
    myaddr_r.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr_r.sin_port = htons(port);
    if (bind(rev_fd, (struct sockaddr *)&myaddr_r, sizeof(myaddr_r)) < 0) {
        perror("bind failed");
        return 0;
    }
}
void receiver(){
    char buf[1024];
    socklen_t addrlen = sizeof(remaddr_r);        /* length of addresses */
    while(1){
        recvlen = recvfrom(rev_fd, buf, 1024, 0, (struct sockaddr *)&remaddr_r, &addrlen);
        string s(buf,strlen(buf));
    }
    return 0;
}
void receiver()
{
    int rev_fd;             /* receive socket */
    int recvlen;
    if ((rev_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        return 0;
    }
    struct sockaddr_in myaddr_r;  /* our address */
    struct sockaddr_in remaddr_r; /* remote address, which will filled by recvfrom */
    socklen_t addrlen = sizeof(remaddr_r);        /* length of addresses */
    memset((char *)&myaddr_r, 0, sizeof(myaddr_r));
    myaddr_r.sin_family = AF_INET;
    myaddr_r.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr_r.sin_port = htons(atoi(argv[1]));
    //myaddr_r.sin_port = htons(4119);
    if (bind(rev_fd, (struct sockaddr *)&myaddr_r, sizeof(myaddr_r)) < 0) {
        perror("bind failed");
        return 0;
    }
    char buf[1024];
    recvlen = recvfrom(rev_fd, buf, 1024, 0, (struct sockaddr *)&remaddr_r, &addrlen);
    char *ipstring = inet_ntoa(remaddr_r.sin_addr);
    int rem_p = ntohs(remaddr_r.sin_port);
    printf("%s\n",rem_p);
    return 0;
}