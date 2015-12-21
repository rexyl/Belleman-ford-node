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
int TIMEOUT=5;
bool writelock=0;
int sendcount = 0;
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
                //std::string s(addressBuffer,INET_ADDRSTRLEN);
                std::string s(addressBuffer,strlen(addressBuffer));
                return s;
            }
            //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            if (!strncmp(ifa->ifa_name,"en0",3) ) {
                //std::string s(addressBuffer,INET_ADDRSTRLEN);
                std::string s(addressBuffer,strlen(addressBuffer));
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
std::unordered_map<string,neighbor_seg> neighbor_list,neighbor_list_backup;
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
    //myaddr_s.sin_port = htons(port);
    if (bind(sendfd, (struct sockaddr *)&myaddr_s, sizeof(myaddr_s)) < 0) {
        perror("bind failed");
        return;
    }
}
void send_string_to(string ip,int port,string &content){
    string content_with_id = content;
    if (content.substr(0,8)!="LINKDOWN" && content.substr(0,6)!="LINKUP") {
        content_with_id = ip+":"+std::to_string(port)+","+content_with_id;
    }
    else{
        int i=0;
    }
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
    while(writelock){}
    writelock = 1;
    if (sendto(sendfd, content_with_id.c_str(), content_with_id.size(), 0, (struct sockaddr *)&remaddr_s, slen)==-1)
        perror("sendto");
    writelock = 0;
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
    //std::cout<<sendcount++<<std::endl;
    for (auto &neigh:neighbor_list)
    {
        send_string_to(neigh.second.dest_ip, neigh.second.dest_port, s);
    }
    gettimeofday(&last_send,NULL);
}
void send_neighbor(string identify){
    string s = "LINKDOWN,"+local_identify+",";
    for (auto &neigh:neighbor_list)
    {
        if(neigh.second.dest()==identify){
            send_string_to(neigh.second.dest_ip, neigh.second.dest_port, s);
            break;
        }
    }
    gettimeofday(&last_send,NULL);
}
void send_neighbor_resume(string identify){
    string s = "LINKUP,"+local_identify+",";
    for (auto &neigh:neighbor_list)
    {
        if(neigh.second.dest()==identify){
            send_string_to(neigh.second.dest_ip, neigh.second.dest_port, s);
            break;
        }
    }
    gettimeofday(&last_send,NULL);
}
string construct_dv(string s,dv_type &n,string &from){
    std::stringstream ss(s);
    string token,identify;
    std::getline(ss, token, ',');
    identify = token;
    std::getline(ss, token, ',');
    from = token;
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
        neighbor_list[from].cost = a->second.cost;
        neighbor_list[from].dest_ip = from_ip;
        neighbor_list[from].dest_port = from_port;
        neighbor_list[from].v = n;
        dvec[from].cost=a->second.cost;
        dvec[from].dest_ip = from_ip;
        dvec[from].dest_port = from_port;
    }
    else{
        it1->second.v = n;
    }//update neighbor list as what you hear
    auto it2 = dvec.find(from); //find neighbor in local dv
    for (auto &x:n)
    {
        auto entry = x.second;
        if(entry.dest() == local_identify){
            continue;
        }
        
        auto it = dvec.find(entry.dest());//for same destination, compare cur with the one neighbor provide
        //auto it2 = dvec.find(from); //find neighbor in local dv
        if(it!=dvec.end()){
            if(INT_MAX-1000 - it2->second.cost > entry.cost && it->second.cost >= it2->second.cost+entry.cost){
                if(it->second.cost > it2->second.cost+entry.cost)  modify =1;
                it->second.cost = it2->second.cost+entry.cost;
                it->second.link = from;
            }
            else if(it->second.link == from){  //however, if you have no choice, you update you path
                //to this dest by selecting from all neighbot again.
                int cur_cost = it->second.cost;
                string cur_link = it->second.link;
                string record;
                int best = INT_MAX-1000;
                for (auto &x:neighbor_list) {
                    if (x.second.dest()==entry.dest()) {    //if dest is already you neighbor
                        best = x.second.cost;
                        record = x.second.dest();
                        break;
                    }
                }
                
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
                if (cur_cost!=best or cur_link!=record) {
                    modify = 1;
                }
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

void counter(){
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
        //std::cout<<"Sleep time "<<sleeptime<<std::endl;
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
    it->second.cost = INT_MAX-1000;
    
    auto it1 = dvec.find(tmp);
    if(it1 == dvec.end()){
        std::cout<<"The link you want to break does not exist\n";
        return;
    }
    send_neighbor(tmp);
    it1->second.cost = INT_MAX-1000;
    
    string record;
    int best;
    for (auto &y:dvec) {
        if(y.second.link == tmp){
            string dest = y.second.dest();
            //set to infi
            y.second.cost = INT_MAX-1000;
            
            string record;
            best = INT_MAX-1000;
            for(auto &x:neighbor_list){
                if(x.second.v.find(tmp) == x.second.v.end() ) //cur neigh do not have a path to dest
                    continue;
                if(INT_MAX-1000 - x.second.cost>x.second.v[dest].cost && x.second.cost+x.second.v[dest].cost < best){
                    best = x.second.cost+x.second.v[dest].cost;
                    record = x.second.dest();
                }
            }
            y.second.cost = best;
            y.second.link = record;    //dest here means ip+port
        }
    }
    //std::cout<<"After break dv is now\n";show_dv();
    send_neighbor();
}
void resume_link(string ip,int port){
    string tmp = ip+":"+std::to_string(port);
    auto it = neighbor_list.find(tmp);
    if(it == neighbor_list.end()){
        std::cout<<"The link you want to resume does not exist\n";
        return;
    }
    it->second.cost = neighbor_list_backup[tmp].cost;
    
    auto it1 = dvec.find(tmp);
    if(it1 == dvec.end()){
        std::cout<<"The link you want to resume does not exist\n";
        return;
    }
    send_neighbor_resume(tmp);
    string record = it->second.dest();
    int best = it->second.cost;
    for(auto &x:neighbor_list){
        if(x.second.v.find(tmp) == x.second.v.end() ) //cur neigh do not have a path to dest
            continue;
        if(INT_MAX-1000 - x.second.cost>x.second.v[tmp].cost && x.second.cost+x.second.v[tmp].cost < best){
            best = x.second.cost+x.second.v[tmp].cost;
            record = x.second.dest();
        }
    }
    dvec[tmp].cost = best;
    dvec[tmp].link = record;
    //std::cout<<"After break dv is now\n";show_dv();
    send_neighbor();
}
void receiver(){
    char buf[1024];
    socklen_t addrlen = sizeof(remaddr_r);        /* length of addresses */
    string identify,ip,port;
    size_t pos;
    dv_type n;
    while(1){
        recvfrom(rev_fd, buf, 1024, 0, (struct sockaddr *)&remaddr_r, &addrlen);
        string s(buf,strlen(buf));
        if(s.substr(0,8)=="LINKDOWN"){
            string downlink = s.substr(9);
            downlink = downlink.substr(0,downlink.find(","));
            auto it = dvec.find(downlink);
            neighbor_list[downlink].cost = INT_MAX - 1000;
            if(it!=dvec.end()){
                it->second.cost = INT_MAX-1000;
                it->second.link = "";
                send_neighbor();
            }
            continue;
        }
        else if(s.substr(0,6)=="LINKUP"){
            string uplink = s.substr(7);
            uplink = uplink.substr(0,uplink.find(","));
            auto it = dvec.find(uplink);
            neighbor_list[uplink].cost = neighbor_list_backup[uplink].cost;
            if(it!=dvec.end()){
                it->second.cost = neighbor_list[uplink].cost;
                it->second.link = uplink;
            }
            continue;
        }
        string from;
        int fromport;
        identify = construct_dv(s, n,from);
        local_identify = identify;
        pos = identify.find(":");
        ip = identify.substr(0,pos);
        port = identify.substr(pos+1);
        pos = from.find(":");
        fromport = stoi(from.substr(pos+1));
        from = from.substr(0,pos);
        update_dv(from, fromport, n);
    }
    return;
}

int main(int argc, char const *argv[])
{
    if(argc<3 || argc%3!=0){
        std::cout<<"Wrong input format!\n";
        return 0;
    }
    std::cout<<"Welcome!\n";
    int local_port = atoi(argv[1]);
    init_recvsock(local_port);
    init_sendsock(0);
    local_identify = getlocal()+":"+std::to_string(local_port);
    //local_identify = "A:"+std::to_string(local_port);
    TIMEOUT = atoi(argv[2]);
    for (int i = 3; i < argc; ++i)
    {
        string ip = argv[i++];
        int port = atoi(argv[i++]);
        int w = atoi(argv[i]);
        string l = ip + ":" + std::to_string(port);
        dvec[l] = dvec_seg(ip,port,w,l);
        neighbor_list[l] = neighbor_seg(ip,port,w,l);
        neighbor_list_backup[l] = neighbor_seg(ip,port,w,l);
    }
    gettimeofday(&last_send,NULL);
    std::thread r(receiver);
    std::thread c(counter);
    r.detach();
    c.detach();
    char buffer[256];
    while(1)
    {
        printf("Command: ");
        bzero(buffer,256);
        fgets(buffer,255,stdin);
        if (!strncmp(buffer, "SHOWRT", 6)) {
            show_dv();
        }
        else if(!strncmp(buffer, "SHOWNT", 6)){
            show_nv();
        }
        else if (!strncmp(buffer, "LINKDOWN", 8)) {
            string tmp(buffer+9);
            size_t pos = tmp.find(" ");
            string ip = tmp.substr(0,pos);
            int port = atoi(tmp.substr(pos).c_str());
            break_link(ip,port);
        }
        else if (!strncmp(buffer, "LINKUP", 6)){
            string tmp(buffer+7);
            size_t pos = tmp.find(" ");
            string ip = tmp.substr(0,pos);
            int port = atoi(tmp.substr(pos).c_str());
            resume_link(ip,port);
        }
        
        else{
            std::cout<<"Bad input\n";
        }
    }
    return 0;
}