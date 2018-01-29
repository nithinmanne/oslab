#include <iostream>
#include <random>
#include <unistd.h>
#include <sys/shm.h>

using namespace std;

#ifndef DEBUG
#define DEBUG false
#endif

#define BUF_SIZE 5
#define WAIT_TIME 30

int prand(minstd_rand);

int main() {
    int NP, NC;
    cout<<"Enter Number of Producers: ";
    cin>>NP;
    cout<<"Enter Number of Consumers: ";
    cin>>NC;
    minstd_rand rand;
    rand.seed((unsigned)time(nullptr));
    auto key = (key_t)rand();
    if(DEBUG) cout<<"Key: "<<key<<endl;
    int shm_id = shmget(key, BUF_SIZE+1, IPC_CREAT | 0666);
    if(shm_id==-1) {
        perror("shmget Failed");
        exit(1);
    }
    if(DEBUG) cout<<"shm_id: "<<shm_id<<endl;

    int *shm_adr;
    shm_adr = (int*)shmat(shm_id, nullptr, 0);
    if(shm_adr==(int*)-1) {
        perror("shmat Failed");
        exit(1);
    }
    if(DEBUG) cout<<"shm_adr: "<<shm_adr<<endl;
    shm_adr[0] = 0;
    for(int i=0;i<BUF_SIZE;i++) shm_adr[i+1] = -1;

    pid_t x = 1;
    vector<pid_t> children;
    int p, c;
    bool producer = true, success = true;
    for(p=0;p<NP&&x>0;p++) children.push_back(x=fork());
    if(x>0) producer = false;
    for(c=0;c<NC&&x>0;c++) children.push_back(x=fork());

    rand.seed((unsigned)((p+c)*time(nullptr)));
    time_t start = time(nullptr);
    int n = 0;
    while(x==0)
        if(producer) {
            int i;
            if(success) {
                n = prand(rand);
                sleep((unsigned)rand()%5);
            }
            success = false;
            while(shm_adr[0]!=p) {
                while(shm_adr[0]!=0)
                    usleep(100);
                shm_adr[0] = p;
                usleep(100);
            }
            for(i=0;i<BUF_SIZE;i++)
                if(shm_adr[i+1]==-1) break;
            if(i==BUF_SIZE){
                shm_adr[0] = 0;
                usleep(1000);
                continue;
            }
            success = true;
            shm_adr[i+1] = n;
            shm_adr[0] = 0;
            printf("Producer %d: %d\tTime: %d\n", p, n, (int)(time(nullptr)-start));
        }
        else {
            if(success) sleep((unsigned)rand()%6);
            success = false;
            int i;
            while(shm_adr[0]!=-c) {
                while(shm_adr[0]!=0)
                    usleep(100);
                shm_adr[0] = -c;
                usleep(100);
            }
            if(shm_adr[1]==-1) {
                shm_adr[0] = 0;
                usleep(1000);
                continue;
            }
            n = shm_adr[1];
            for(i=1;i<BUF_SIZE;i++)
                shm_adr[i] = shm_adr[i+1];
            shm_adr[BUF_SIZE] = -1;
            shm_adr[0] = 0;
            success = true;
            printf("Consumer %d: %d\tTime: %d\n", c, n, (int)(time(nullptr)-start));
        }
    for(int i=0;i<WAIT_TIME;i++) {
        sleep(1);
        if(DEBUG) printf("shm = %d  %d  %d  %d  %d  %d\n", shm_adr[0], shm_adr[1], shm_adr[2], shm_adr[3], shm_adr[4], shm_adr[5]);
    }
    //sleep(WAIT_TIME);
    for(pid_t &i: children) kill(i, 9);
}

int prand(minstd_rand rand) {
    while(true) {
        int r = (int)rand(), p = 1;
        for(int i=2;i<r;i++)
            if(r%i==0) {
                p = 0;
                break;
            }
        if(p) return r;
    }
}