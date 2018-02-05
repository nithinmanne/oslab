#include <iostream>
#include <random>

using namespace std;

#ifndef DEBUG
#define DEBUG false
#endif

typedef struct {
    int ar_tm, fn_tm;
    int cpu;
    bool cmp;
} prcs;

#define MEAN .3

prcs *copy(const prcs*, int);
void fcfs(prcs*, int);
void sjf(prcs*, int);
void rr(prcs*, int, int);

int main(int argc, char *argv[]) {
    int N;
    if(argc==1) {
        cout<<"Enter value of N: ";
        cin>>N;
    }
    else
        N = atoi(argv[1]);
    prcs *process;
    process = (prcs*)malloc(N*sizeof(prcs));
    int ct = 0;
    minstd_rand rand;
    rand.seed(time(nullptr));
    for(int i=0;i<N;i++) {
        process[i].cpu = 1+rand()%20;
        process[i].ar_tm = ct;
        process[i].fn_tm = -1;
        process[i].cmp = false;
        ct += (int)round(-1.0/MEAN*log((double)rand()/rand.max()));
    }

    prcs *fcfspr = copy(process, N);
    fcfs(fcfspr, N);
    float fcfsatn = 0;
    for(int i=0;i<N;i++)
        fcfsatn += fcfspr[i].fn_tm - fcfspr[i].ar_tm;
    fcfsatn /= N;
    if(argc==1) printf("FCFS ATN = %f\n", fcfsatn);
    else printf("%f\n", fcfsatn);
    if(argc==1) for(int i=0;i<N;i++) printf("%d\t%d\t%d\n", fcfspr[i].ar_tm, process[i].cpu, fcfspr[i].fn_tm);


    prcs *sjfpr = copy(process, N);
    sjf(sjfpr, N);
    float sjfatn = 0;
    for(int i=0;i<N;i++)
        sjfatn += sjfpr[i].fn_tm - sjfpr[i].ar_tm;
    sjfatn /= N;
    if(argc==1) printf("SJF ATN = %f\n", sjfatn);
    else printf("%f\n", sjfatn);
    if(argc==1) for(int i=0;i<N;i++) printf("%d\t%d\t%d\n", sjfpr[i].ar_tm, process[i].cpu, sjfpr[i].fn_tm);


    prcs *rr1pr = copy(process, N);
    rr(rr1pr, N, 1);
    float rr1atn = 0;
    for(int i=0;i<N;i++)
        rr1atn += rr1pr[i].fn_tm - rr1pr[i].ar_tm;
    rr1atn /= N;
    if(argc==1) printf("RR with 1 ATN = %f\n", rr1atn);
    else printf("%f\n", rr1atn);
    if(argc==1) for(int i=0;i<N;i++) printf("%d\t%d\t%d\n", rr1pr[i].ar_tm, process[i].cpu, rr1pr[i].fn_tm);

    prcs *rr2pr = copy(process, N);
    rr(rr2pr, N, 2);
    float rr2atn = 0;
    for(int i=0;i<N;i++)
        rr2atn += rr2pr[i].fn_tm - rr2pr[i].ar_tm;
    rr2atn /= N;
    if(argc==1) printf("RR with 2 ATN = %f\n", rr2atn);
    else printf("%f\n", rr2atn);
    if(argc==1) for(int i=0;i<N;i++) printf("%d\t%d\t%d\n", rr2pr[i].ar_tm, process[i].cpu, rr2pr[i].fn_tm);

    prcs *rr5pr = copy(process, N);
    rr(rr5pr, N, 5);
    float rr5atn = 0;
    for(int i=0;i<N;i++)
        rr5atn += rr5pr[i].fn_tm - rr5pr[i].ar_tm;
    rr5atn /= N;
    if(argc==1) printf("RR with 5 ATN = %f\n", rr5atn);
    else printf("%f\n", rr5atn);
    if(argc==1) for(int i=0;i<N;i++) printf("%d\t%d\t%d\n", rr5pr[i].ar_tm, process[i].cpu, rr5pr[i].fn_tm);
}

void fcfs(prcs *process, int N) {
    int ct = 0;
    for(int i=0;i<N;i++) {
        ct += process[i].cpu;
        process[i].fn_tm = ct;
    }
}

void sjf(prcs *process, int N) {
    int ct = 0, st;
    while(true) {
        st = -1;
        for(int i=0;i<N;i++) {
            if(process[i].ar_tm>ct) break;
            if(process[i].cpu==0) continue;
            if(st==-1 || process[i].cpu<process[st].cpu) st = i;
        }
        if(st!=-1) process[st].cpu--;
        ct++;
        if(st!=-1 && process[st].cpu==0) process[st].fn_tm = ct;
        bool done=true;
        for(int i=0;i<N;i++)
            if(process[i].cpu>0) {
                done=false;
                break;
            }
        if(done) break;
    }
}

void rr(prcs *process, int N, int q) {
    int ct = 0, cp = -1;
    while(true) {
        bool done=true;
        for(int i=0;i<N;i++)
            if(process[i].cpu>0) {
                done=false;
                break;
            }
        if(done) break;
        bool nop=true;
        for(int i=0;i<N;i++) {
            if(process[i].ar_tm>ct) break;
            if(process[i].cpu>0) {
                nop=false;
                break;
            }
        }
        if(nop) {
            ct++;
            continue;
        }
        if(cp+1==N || process[cp+1].ar_tm>ct) cp=0;
        else cp++;
        if(process[cp].cpu==0) continue;
        if(process[cp].cpu>q) {
            ct += q;
            process[cp].cpu -= q;
        }
        else {
            ct += process[cp].cpu;
            process[cp].cpu = 0;
            process[cp].fn_tm = ct;
        }
    }
}

prcs *copy(const prcs *orig, int N) {
    prcs *copy = (prcs*)malloc(N*sizeof(prcs));
    for(int i=0;i<N;i++) {
        copy[i].ar_tm = orig[i].ar_tm;
        copy[i].fn_tm = orig[i].fn_tm;
        copy[i].cpu = orig[i].cpu;
        copy[i].cmp = orig[i].cmp;
    }
    return copy;
}
