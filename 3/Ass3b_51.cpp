/*
Please compile with -std=c++11. This program is written in an IDE, where C++11 is the default, as it should be. I didn't check for compatibility with C++98.
*/
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include <random>
#include <sys/wait.h>
#include <pthread.h>

using namespace std;

int *status;
int *kstatus;
int N;
pthread_t *thr;
pthread_mutex_t status_mutex;

int main() {
    cout<<"Enter N: ";
    cin>>N;
    status = (int*)malloc(N*sizeof(int));
    kstatus = (int*)malloc(N*sizeof(int));
    for(int i=0;i<N;i++) status[i] = 0;
    for(int i=0;i<N;i++) kstatus[i] = 0;
    thr = (pthread_t*)malloc((N+2)*sizeof(pthread_t));
    pthread_mutex_init(&status_mutex, nullptr);
    for(int ityp=0;ityp<N+2;ityp++)
        if(pthread_create(&thr[ityp], nullptr, [](void* arg) {
            auto typ = (long)arg;
            if(typ==0) {
                int cid = 0;
                sleep(1);
                while(true) {
                    pthread_mutex_lock(&status_mutex);
                    if(status[cid]!=-1) {
                        pthread_mutex_unlock(&status_mutex);
                        pthread_kill(thr[cid + 2], SIGUSR2);
                        pthread_mutex_lock(&status_mutex);
                        status[cid] = 1;
                        pthread_mutex_unlock(&status_mutex);
                        sleep(1);
                        if(kstatus[cid]==1) {
                            pthread_mutex_lock(&status_mutex);
                            status[cid] = -1;
                            pthread_mutex_unlock(&status_mutex);
                        }
                        else {
                            pthread_kill(thr[cid + 2], SIGUSR1);
                            pthread_mutex_lock(&status_mutex);
                            status[cid] = 0;
                            pthread_mutex_unlock(&status_mutex);
                        }
                    }
                    else
                        pthread_mutex_unlock(&status_mutex);
                    cid=(cid+1)%N;
                    bool done = true;
                    pthread_mutex_lock(&status_mutex);
                    for(int i=0;i<N;i++)
                        if(status[i]!=-1) {
                            done=false;
                            break;
                        }
                    pthread_mutex_unlock(&status_mutex);
                    if(done) break;
                }
                return (void*)nullptr;
            }
            else if(typ==1) {
                int *ostatus;
                ostatus = (int*)malloc(N*sizeof(int));
                pthread_mutex_lock(&status_mutex);
                for(int i=0;i<N;i++) ostatus[i] = status[i];
                pthread_mutex_unlock(&status_mutex);
                while(true) {
                    for(int i=0;i<N;i++) {
                        pthread_mutex_lock(&status_mutex);
                        bool chk = status[i]==ostatus[i];
                        pthread_mutex_unlock(&status_mutex);
                        if (chk) continue;
                        pthread_mutex_lock(&status_mutex);
                        ostatus[i]=status[i];
                        pthread_mutex_unlock(&status_mutex);
                        printf("Worker %d ", i);
                        if(ostatus[i]==-1) printf("Terminated\n");
                        if(ostatus[i]==0) printf("Paused\n");
                        if(ostatus[i]==1) printf("Resumed\n");
                        fflush(stdout);
                        usleep(1000);
                    }
                    usleep(10000);
                    bool done = true;
                    for(int i=0;i<N;i++)
                        if(ostatus[i]!=-1) {
                            done=false;
                            break;
                        }
                    if(done) break;
                }
                return (void*)nullptr;
            }
            signal(SIGUSR1, [](int){});
            signal(SIGUSR2, [](int){});
            pause();
            int rand_arr[1000];
            minstd_rand rand;
            rand.seed((unsigned)typ);
            for (int &i : rand_arr) i = (int)rand();
            qsort(rand_arr, 1000, sizeof(int), [](const void *a, const void *b) {
                int arg1 = *static_cast<const int*>(a);
                int arg2 = *static_cast<const int*>(b);
                if(arg1 < arg2) return -1;
                if(arg1 > arg2) return 1;
                return 0;
            });
            int sTime = 1 + (int)rand()%10;
            time_t t = time(nullptr);
            while(time(nullptr)-t<sTime);
            pthread_mutex_lock(&status_mutex);
            kstatus[typ-2] = 1;
            pthread_mutex_unlock(&status_mutex);
            return (void*)nullptr;
        }, (void*)(long)ityp));
    pthread_join(thr[0], nullptr);
    sleep(1);
}