#include <jni.h>
#include "libs/log.h"
#include "libs/getTime.h"
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <string>
#include <random>
#include <netinet/tcp.h>
#include <set>
#include <sys/epoll.h>
#include <fstream>
#include <signal.h>
#include <unistd.h>

int REMOTE_PORT = 7000;
int epfd , local_sockfd = 0;
epoll_event evlist[20];
static const int packet[7] = {4999, 99, 11, 11, 999, 999, 9};

sockaddr_in remote_addr;
static long serviceid;
//static uint64_t lastRecvTime = ~0;

void update();
void finish();

void rst_handle(){
    // Handle the network-change signal
    // close and delete the old connection
//    LOGI("RESET !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    if (!local_sockfd)
        return;
    epoll_ctl(epfd,EPOLL_CTL_DEL,local_sockfd,NULL);
    close(local_sockfd);
    local_sockfd = 0;

    // sleep 3s to reconnect
//    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    // get a new fd and reconnect
    local_sockfd = socket(AF_INET,SOCK_STREAM,0);

    int flag = 1;
    setsockopt(local_sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));
    connect(local_sockfd, (sockaddr* )&remote_addr , sizeof(remote_addr));

    // add the new clnt-fd to the epoll
    epoll_event ev;
    ev.data.fd = local_sockfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,local_sockfd,&ev);
    LOGI("reconnect finished!");
}


void init_socket(const char* ip){
    epfd = epoll_create(2);
    local_sockfd = socket(AF_INET,SOCK_STREAM,0);

    int flag = 1;

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(ip);
    remote_addr.sin_port = htons(REMOTE_PORT);

    setsockopt(local_sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));
    if(connect(local_sockfd, (sockaddr* )&remote_addr , sizeof(remote_addr)) != 0){
        LOGE("connection failed!\n");
//        cout << "connection failed" <<endl;
    }

    epoll_event ev;
    ev.data.fd = local_sockfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,local_sockfd,&ev);

    LOGI("init socket success!\n");
//    cout<<"init socket success"<<endl;
}

void service_thread(int op, int time){
//    signal(SIGUSR1,rst_handle);
    char msgbuffer[8];
    std::set<int> sss;
    msgbuffer[0] = op + '0';

    if (!local_sockfd)
        exit(-1);
    send(local_sockfd, msgbuffer,2,0);
    // cout<<"in service thread"<<endl;
    int evtnum , recvn;

    //std::string filename = std::string("/data/data/com.example.tcptestapp/result") + std::to_string(op) + ".txt";
    //FILE* file = fopen(filename.c_str(), "w+");
    char recvbuffer[4096];
//    if (op == 6) {
//        std::this_thread::sleep_for(std::chrono::seconds(3600));
//        rst_handle();
//    }
    int blockTime = (op == 6) ? time : 180000;
    //blockTime = std::max(time, 180000);
    unsigned long long start = getMillis();
    bool loseConnect = false;
    while(1){
        evtnum = epoll_wait(epfd,evlist,20, blockTime);
        if (op == 6 && (getMillis() - start >= 3650000)) {
            close(local_sockfd);
            //fclose(file);
            LOGI("finish!!!");
            finish();
            return;
        }
        if (evtnum == 0) {
            if (loseConnect) {
                rst_handle();
                loseConnect = false;
                continue;
            }
            std::string hdata = "h";
            hdata.append(1399, 'h');
            send(local_sockfd , hdata.c_str() ,hdata.size(), 0);
            LOGI("send heartbeat!");
            loseConnect = true;
        }
        for(int i = 0;i < evtnum ; i++){
            loseConnect = false;
            recvn = recv(evlist[i].data.fd,recvbuffer,4096,0);
            if (recvn && recvbuffer[0] == 'h') {
                LOGI("recv heartBeat echo!");
                continue;
            }
            //cout<<recvn<<endl;
            uint64_t recvTime = getMicros();
            if(recvn > 30){
                //LOGI("recv %d\n", recvn);
                std::string data = std::string(recvbuffer, recvn);
                if (data.length() > 1000)
                    data = data.substr(0, data.find_first_of("+"));
                int number = atoi(data.substr(data.find_first_of(":") + 1).c_str());
                if (number > packet[op]) {
                    LOGI("test finished!");
                    close(evlist[i].data.fd);
                    //fclose(file);
                    finish();
                    return;
                }
                if (sss.find(number) != sss.end())
                    continue;
                sss.insert(number);
                //data += " recvTime:" + std::to_string(recvTime);
                //LOGI("%s", data.c_str());

                //fprintf(file, "%s\n", data.c_str());
//                if (sss.size() % 10 == 0) {
//                    fflush(file);
//                }
                update();
//                //std::string packet = data + " ackTime:" + std::to_string(getMillis());
//                std::string packet = data.substr(0,data.find_first_of("s")) + " ackTime:" + std::to_string(getMicros());
                data.append(1000, '+');
                send(evlist[i].data.fd , data.c_str() ,data.size(), 0);
            }
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_tcptestapp_NetworkChangeReceiver_reset(JNIEnv* env, jobject /* this */thiz){
//    // Send a signal to service_thread periodlly , each signal indicate means a network change
//    int sigs = 10;
//    std::default_random_engine e;
//    // send a signal every 10-15s
//    std::uniform_int_distribution<unsigned> u(10000, 15000);
//    while(sigs--){
//        int gap = u(e);
//        std::this_thread::sleep_for(std::chrono::milliseconds(gap));
//        pthread_kill(serviceid,SIGUSR1);
//    }
    LOGI("JNI DO Re-connect!!!\n");
    rst_handle();
//    int no = pthread_kill(serviceid, SIGUSR1);
//    if (no == ESRCH)
//        LOGI("thread not exist.");
//    else if (no == EINVAL)
//        LOGI("signal not useable.");
//    else LOGI("kill number : %d");
}

static JavaVM* jvm = nullptr;
static jobject object = nullptr;

extern "C" JNIEXPORT jint JNICALL
Java_com_example_tcptestapp_FirstTest_startTest(JNIEnv* env, jobject /* this */thiz,
        jstring ip, jint op, jint time) {
    env->GetJavaVM(&jvm);
    object = env->NewGlobalRef(thiz);
    const char* REMOTE_IP = env->GetStringUTFChars(ip, nullptr);
   // lastRecvTime = ~0;
    init_socket(REMOTE_IP);
//    LOGI("init");
    std::thread ser(service_thread, op, time);
    serviceid = ser.native_handle();
    LOGI("id::::: %ld", serviceid);
    ser.detach();
    return 0;
}

void update() {
    JNIEnv* env;
    jvm->AttachCurrentThread(&env, nullptr);

    if (env == nullptr) {
        LOGI("env null!!!\n");
        return;
    }
    jclass clazz = env->GetObjectClass(object);

    if (clazz == nullptr) {
        LOGI("class null!\n");
        return;
    }

    jmethodID callBackMethod = env->GetStaticMethodID(clazz, "updateInfo", "()V");
    if (callBackMethod == nullptr) {
        LOGI("method null!\n");
        return;
    }

    env->CallStaticVoidMethod(clazz, callBackMethod);
}

void finish() {
    JNIEnv* env;
    jvm->AttachCurrentThread(&env, nullptr);

    if (env == nullptr) {
        LOGI("env null!!!\n");
        return;
    }
    jclass clazz = env->GetObjectClass(object);

    if (clazz == nullptr) {
        LOGI("class null!\n");
        return;
    }

    jmethodID callBackMethod = env->GetStaticMethodID(clazz, "finishTest", "()V");
    if (callBackMethod == nullptr) {
        LOGI("method null!\n");
        return;
    }

    env->CallStaticVoidMethod(clazz, callBackMethod);
    jvm->DetachCurrentThread();
}