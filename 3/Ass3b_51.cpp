#include <iostream>
#include <pthread.h>

using namespace std;

void *thr_func(void*);

int main() {
    cout<<"Enter N: ";
    int N;
    cin>>N;
    pthread_t *thr;
    thr = (pthread_t*)malloc((N+2)*sizeof(pthread_t));
    for(int i=0;i<N+2;i++)
        if(pthread_create(&thr[i], nullptr, thr_func, (void*)(long)i));
}

void *thr_func(void *type) {
    long typ = (long)type;
    if(typ==0) cout<<"Scheduler\n";
    else if(typ==1) cout<<"Reporter\n";
    else cout<<"Hi from Worker "<<(typ-2)<<"\n";
    return (void*)nullptr;
}
