#include <iostream>
#include <cstdlib>
#include <unistd.h>
using namespace std;

int cmp(const void *a, const void *b) {
    return *(const int*)a  - *(const int*)b;
}

#define COUNT 100

int main() {
    pid_t pid[3] = {0, 0, 0}; int pip[3][2];
    pipe(pip[0]); pipe(pip[1]); pipe(pip[2]);
    int a_pip = pip[0][1]; pid[0] = fork();
    if(pid[0]!=0) {
        a_pip = pip[1][1]; pid[1] = fork();
    }
    if(pid[1]!=0) {
        a_pip = pip[2][1]; pid[2] = fork();
    }
    if(pid[2]==0) {
        int ar[COUNT]; srand((unsigned)a_pip);
        for(int i=0;i<COUNT;i++) ar[i] = rand();
        qsort(ar, COUNT, sizeof(int), cmp);
        for(int i=0;i<COUNT;i++) write(a_pip, &ar[i], sizeof(int));
    }
    else {
        int ar[3*COUNT], r[3]={1, 1, 1}, c[3]={0, 0, 0}, t[3], cmn = -1;
        for(int i=0;i<3*COUNT;i++) {
            for(int j=0;j<3;j++)
                if(r[j]==1&&c[j]<COUNT) {
                    read(pip[j][0], &t[j], sizeof(int));
                    r[j] = 0; c[j]++;
                    if(cmn==-1||t[j]<t[cmn]) cmn = j;
                }
            ar[i] = t[cmn]; r[cmn] = 1;
            bool u1 = c[(cmn+1)%3]<COUNT||r[(cmn+1)%3]==0, u2 = c[(cmn+2)%3]<COUNT||r[(cmn+2)%3]==0;
            if(u1&&u2)
                if(t[(cmn+1)%3]<t[(cmn+2)%3]) cmn = (cmn+1)%3;
                else cmn = (cmn+2)%3;
            else if(u1) cmn = (cmn+1)%3;
            else if(u2) cmn = (cmn+2)%3;
            cout<<ar[i]<<"\n";
        }
    }
}