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
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sstream>

typedef std::string string;

string local_identify;
timeval last_send;
class dvec_seg
{
public:
    string dest_ip;
    int dest_port;
    int cost;
    string link;
    dvec_seg(){
        dest_ip = "local";
        dest_port = 0;
        cost = 0;
        link = "local";
    }
    
    dvec_seg(string ip,int dest_p, int c, string l){
        dest_ip = ip;
        dest_port = dest_p;
        cost = c;
        link = l;
    }
    //~dvec_seg();
    string dest(){
        return dest_ip + ":" + std::to_string(dest_port);
    }
};
class neighbor_seg:public dvec_seg
{
public:
    std::unordered_map<string,dvec_seg> v;
    neighbor_seg(){
        dest_ip = "local";
        dest_port = 0;
        cost = 0;
        link = "local";
    }
    neighbor_seg(string ip,int dest_p, int c, string l){
        dest_ip = ip;
        dest_port = dest_p;
        cost = c;
        link = l;
    }
    //~neighbor_seg();
    string dest(){
        return dest_ip + ":" + std::to_string(dest_port);
    }
};
long diff_ms(timeval t1, timeval t2){
    return (((t1.tv_sec - t2.tv_sec) * 1000000) +
            (t1.tv_usec - t2.tv_usec))/1000;
}
std::string getlocal() {
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    
    getifaddrs(&ifAddrStruct);
    
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if (!strncmp(ifa->ifa_name,"en0",3) ) {
                std::string s(addressBuffer,INET_ADDRSTRLEN);
                return s;
            }
            //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            if (!strncmp(ifa->ifa_name,"en0",3) ) {
                std::string s(addressBuffer,INET_ADDRSTRLEN);
                return s;
            }
            //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return "NONE";
}
typedef std::unordered_map<string,dvec_seg> dv_type;
dv_type dvec;
std::unordered_map<string,neighbor_seg> neighbor_list;

void send_neighbor(){
    for (auto &neigh:neighbor_list)
    {
        //modify write socket ip as neigh.dest_ip, port as dest_port
        //send dvec through socket
        //        if (sendto(fd, resend_buffer_v[i].buffer, 20+BUFSIZE, 0, (struct sockaddr *)&remaddr, slen)==-1)
        //            perror("sendto");
    }
    gettimeofday(&last_send,NULL);
}
void construct_dv(string s,dv_type &n){

}
void update_dv(string from_ip,int from_port,dv_type &n){
    //encapsure vector we received as neighbor_seg sothat ez to update
    bool modify = 0;
    string from = from_ip+":"+std::to_string(from_port);
    auto it1 = neighbor_list.find(from);
    
    if (it1 == neighbor_list.end()) {
        auto a = n.find(local_identify);
        neighbor_seg s(from_ip,from_port,a->second.cost , from);
        std::pair<string,neighbor_seg> p(from,s);
        neighbor_list.insert(p);
    }
    else{
        it1->second.v = n;
    }//update neighbor list as what you hear
    auto it2 = dvec.find(from); //find neighbor in local dv
    for (auto &x:n)
    {
        auto entry = x.second;
        if(entry.dest() == local_identify)
            continue;
        auto it = dvec.find(entry.dest());//for same destination, compare cur with the one neighbor provide
        //auto it2 = dvec.find(from); //find neighbor in local dv
        if(it!=dvec.end()){
            if(INT_MAX - it2->second.cost >= entry.cost && it->second.cost > it2->second.cost+entry.cost){
                it->second.cost = it2->second.cost+entry.cost;
                it->second.link = from;
                modify =1;
            }
            else if(it->second.link == from){  //however, if you have no choice, you update you path
                //to this dest by selecting from all neighbot again.
                string record;
                int best = INT_MAX;
                for(auto &x:neighbor_list){
                    if(x.second.v.find(entry.dest()) == x.second.v.end() ) //cur neigh do not have a path to dest
                        continue;
                    if(x.second.cost+x.second.v[entry.dest()].cost < best){
                        best = x.second.cost+x.second.v[entry.dest()].cost;
                        record = x.second.dest();
                    }
                }
                it->second.cost = best;
                it->second.link = record;    //dest here means ip+port
                modify = 1;
            }
        }
        else{
            dvec_seg ds(entry.dest_ip,entry.dest_port,entry.cost+it2->second.cost,from);
            std::pair<string, dvec_seg> tmp(entry.dest(),ds);
            dvec.insert(tmp);
        }
        if(modify){
            send_neighbor();
        }
    }
    return;
}

void counter(int TIMEOUT){
    sleep(TIMEOUT);
    timeval now;
    int sleeptime;
    while(1){
        gettimeofday(&now, NULL);
        long sec = diff_ms(now, last_send);   //from last send time to now
        if (sec > TIMEOUT*1000)
        {
            send_neighbor();
        }
        sleeptime = (int)(TIMEOUT -sec/1000);
        sleep(sleeptime);
    }
    return;
}
void show_dv(){
    time_t now = time(NULL);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    string cur_time(buf, strlen(buf));
    std::cout<<"Current time is "<<cur_time<<"\nDistance vector list is:\n";
    for (auto &x:dvec)
    {
        std::cout<<"Destination = "<<x.second.dest()<<", Cost = "<<x.second.cost<<
        ", Link = ("<<x.second.link<<")\n";
    }
    return;
}
void show_nv(){
    std::cout<<"NV\n";
    time_t now = time(NULL);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    string cur_time(buf, strlen(buf));
    std::cout<<"Current time is "<<cur_time<<"\nDistance vector list is:\n";
    for (auto &x:neighbor_list)
    {
        std::cout<<"Destination = "<<x.second.dest()<<", Cost = "<<x.second.cost<<
        ", Link = ("<<x.second.link<<")\n";
    }
    return;
}
void break_link(string ip,int port){
    string tmp = ip+":"+std::to_string(port);
    auto it = neighbor_list.find(tmp);
    if(it == neighbor_list.end()){
        std::cout<<"The link you want to break does not exist\n";
        return;
    }
    it->second.cost = INT_MAX;
    
    auto it1 = dvec.find(tmp);
    if(it1 == dvec.end()){
        std::cout<<"The link you want to break does not exist\n";
        return;
    }
    it1->second.cost = INT_MAX;
    
    string record;
    int best;
    for (auto &y:dvec) {
        if(y.second.link == tmp){
            string dest = y.second.dest();
            best = INT_MAX;
            //set to infi
            y.second.cost = INT_MAX;
            
            string record;
            int best = INT_MAX;
            for(auto &x:neighbor_list){
                if(x.second.v.find(tmp) == x.second.v.end() ) //cur neigh do not have a path to dest
                    continue;
                if(INT_MAX - x.second.cost>x.second.v[dest].cost && x.second.cost+x.second.v[dest].cost < best){
                    best = x.second.cost+x.second.v[dest].cost;
                    record = x.second.dest();
                }
            }
            y.second.cost = best;
            y.second.link = record;    //dest here means ip+port
        }
    }
    
    
    send_neighbor();
}
void b(){
    dv_type tmp;
    dvec_seg ds1("C",4117,10,"C:4117");
    dvec_seg ds2("D",4118,5,"D:4118");
    dvec_seg ds3("A",4115,5,"A:4115");
    tmp.insert({"C:4117",ds1});
    tmp.insert({"D:4118",ds2});
    tmp.insert({"A:4115",ds3});
    update_dv("B",4116, tmp);
    return;
}
void d(){
    dv_type tmp;
    dvec_seg ds11("A",4115,30,"B:4116");
    dvec_seg ds22("B",4116,5,"B:4116");
    dvec_seg ds33("C",4117,15,"B:4116");
    tmp.insert({"A:4115",ds11});
    tmp.insert({"B:4116",ds22});
    tmp.insert({"C:4117",ds33});
    update_dv("D",4118, tmp);
    return;
}
int main(int argc, char const *argv[])
{
    if(argc<3 || argc%3!=0){
        std::cout<<"Wrong input format!\n";
        return 0;
    }
    int local_port = atoi(argv[1]);
    local_identify = getlocal()+std::to_string(local_port);
    int TIMEOUT = atoi(argv[2]);
    for (int i = 3; i < argc; ++i)
    {
        string ip = argv[i++];
        int port = atoi(argv[i++]);
        int w = atoi(argv[i]);
        string l = ip + ":" + std::to_string(port);
        dvec[l] = dvec_seg(ip,port,w,l);
        neighbor_list[l] = neighbor_seg(ip,port,w,l);
    }
    d();
    show_dv();
    b();
    show_dv();
    //show_nv();
    break_link("B", 4116);
    show_dv();
    //show_nv();
    return 0;
}