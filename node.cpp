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
int sendfd;
struct sockaddr_in myaddr_s, remaddr_s,myaddr_r,remaddr_r;
int rev_fd;             /* receive socket */
void init_recvsock(int port){
    if ((rev_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        return;
    }
    socklen_t addrlen = sizeof(remaddr_r);        /* length of addresses */
    memset((char *)&myaddr_r, 0, sizeof(myaddr_r));
    myaddr_r.sin_family = AF_INET;
    myaddr_r.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr_r.sin_port = htons(port);
    if (bind(rev_fd, (struct sockaddr *)&myaddr_r, sizeof(myaddr_r)) < 0) {
        perror("bind failed");
        return;
    }
}
void init_sendsock(int port){
    if ((sendfd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
        printf("socket created\n");
    memset((char *)&myaddr_s, 0, sizeof(myaddr_s));
    myaddr_s.sin_family = AF_INET;
    myaddr_s.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr_s.sin_port = htons(port);
//    if (bind(sendfd, (struct sockaddr *)&myaddr_s, sizeof(myaddr_s)) < 0) {
//        perror("bind failed");
//        return;
//    }
}
void send_string_to(string ip,int port,string &content){
    string server = ip;
    memset((char *) &remaddr_s, 0, sizeof(remaddr_s));
    int slen=sizeof(remaddr_s);
    remaddr_s.sin_family = AF_INET;
    remaddr_s.sin_port = htons(port);
    if (inet_aton(server.c_str(), &remaddr_s.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    //std::string s = "here we go with first one!";
    if (sendto(sendfd, content.c_str(), content.size(), 0, (struct sockaddr *)&remaddr_s, slen)==-1)
        perror("sendto");
}

void construct_buffer(string &s){
    s.clear();
    s += local_identify+",";
    for (auto &x:dvec)
    {
        dvec_seg ds = x.second;
        s += ds.dest_ip+","+std::to_string(ds.dest_port)+","+std::to_string(ds.cost)+","+ds.link+",";
    }
    s.pop_back();
    return;
}
void send_neighbor(){
    string s;
    construct_buffer(s);
    for (auto &neigh:neighbor_list)
    {
        send_string_to(neigh.second.dest_ip, neigh.second.dest_port, s);
    }
    gettimeofday(&last_send,NULL);
}
string construct_dv(string s,dv_type &n){
    std::stringstream ss(s);
    string token,identify;
    std::getline(ss, token, ',');
    identify = token;
    n.clear();
    int i = 0;
    string tmp[4];
    while (std::getline(ss, token, ',')) {
        tmp[i]=token;
        if (++i>3) {
            i = 0;
            for (int j=0; j<4; j++) {
                dvec_seg ds(tmp[0],std::stoi(tmp[1]),std::stoi(tmp[2]),tmp[3]);
                n[ds.dest()] = ds;
            }
        }
        //std::cout<<token<<std::endl;
    }
    return identify;
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
void receiver(){
    char buf[1024];
    socklen_t addrlen = sizeof(remaddr_r);        /* length of addresses */
    string dest,ip,port;
    size_t pos;
    dv_type n;
    while(1){
        recvfrom(rev_fd, buf, 1024, 0, (struct sockaddr *)&remaddr_r, &addrlen);
        string s(buf,strlen(buf));
        dest = construct_dv(s, n);
        pos = dest.find(":");
        ip = dest.substr(0,pos+1);
        port = dest.substr(pos+1);
        update_dv(ip, std::stoi(port), n);
    }
    return;
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
    //local_identify = getlocal()+":"+std::to_string(local_port);
    local_identify = "A:"+std::to_string(local_port);
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
    gettimeofday(&last_send,NULL);
//    d();
//    show_dv();
//    b();
//    show_dv();
//    //show_nv();
//    break_link("B", 4116);
//    show_dv();
//    //show_nv();
//    string trash;
//    construct_buffer(trash);
//    std::cout<<trash<<std::endl;
//    dv_type tm;
//    dvec.clear();
//    construct_dv(trash, dvec);
//    show_dv();
    return 0;
}