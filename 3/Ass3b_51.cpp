#include <iostream>
#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include <random>
#include <sys/wait.h>
#include <pthread.h>

using namespace std;

void *thr_func(void*);
void handler(int);
int *status;

int main() {
    cout<<"Enter N: ";
    int N;
    cin>>N;
    status = (int*)malloc(N*sizeof(int));
    pthread_t *thr;
    thr = (pthread_t*)malloc((N+2)*sizeof(pthread_t));
    for(int i=0;i<N+2;i++)
        if(pthread_create(&thr[i], nullptr, thr_func, (void*)(long)i));
    pthread_join(thr[0], nullptr);
}


void *thr_func(void *type) {
    auto typ = (long)type;
    if(typ==0) {
        printf("Scheduler\n");
        for(int i=0;i<100000;i++) {
            status[i%10]=i;
        }
        sleep(5);
        printf("Scheduler\n");
        return (void*)nullptr;
    }
    else if(typ==1) {
        printf("Reporter\n");
        for(int i=0;i<100000;i++) {
            for(int j=0;j<10;j++) {
                printf("%d ", status[j]);
            }
            printf("\n");
        }
        sleep(20);
        printf("Reporter\n");
        return (void*)nullptr;
    }
    signal(SIGUSR1, handler);
    signal(SIGUSR2, handler);
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
    return (void*)nullptr;
}

void handler(int SIG) {
    if(SIG==SIGUSR1)
        pause();
}