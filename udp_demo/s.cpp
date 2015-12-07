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
int sendfd;
struct sockaddr_in myaddr, remaddr;
void init_sendsock(int port){
    if ((sendfd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        printf("socket created\n");
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(port);
    if (bind(sendfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }
}
void send_string_to(string ip,int port,string content){
    string server = ip;
    memset((char *) &remaddr, 0, sizeof(remaddr));
    int slen=sizeof(remaddr);
    remaddr.sin_family = AF_INET;
    remaddr.sin_port = htons(port);
    if (inet_aton(server.c_str(), &remaddr.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    //std::string s = "here we go with first one!";
    if (sendto(sendfd, content.c_str(), content.size(), 0, (struct sockaddr *)&remaddr, slen)==-1)
        perror("sendto");
}
int main(int argc, char const *argv[])
{
    //int sendfd;             /* receive socket */
    int recvlen;
    /* create a socket */
    if ((sendfd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        printf("socket created\n");
    std::string server = "127.0.0.1";
    /* bind it to all local addresses and pick any port number */
    struct sockaddr_in myaddr, remaddr;
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(5000);
    
    if (bind(sendfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }
    
    /* now define remaddr, the address to whom we want to send messages */
    /* For convenience, the host address is expressed as a numeric IP address */
    /* that we will convert to a binary format via inet_aton */
    
    memset((char *) &remaddr, 0, sizeof(remaddr));
    int slen=sizeof(remaddr);
    remaddr.sin_family = AF_INET;
    remaddr.sin_port = htons(4119);
    if (inet_aton(server.c_str(), &remaddr.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    std::string s = "here we go with first one!";
    if (sendto(sendfd, s.c_str(), s.size(), 0, (struct sockaddr *)&remaddr, slen)==-1)
        perror("sendto");

    remaddr.sin_port = htons(4120);
    s = "here we go with second one!";
    if (sendto(sendfd, s.c_str(), s.size(), 0, (struct sockaddr *)&remaddr, slen)==-1)
        perror("sendto");
    
    return 0;
}