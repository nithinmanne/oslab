#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;

int cmp(const void *a, const void *b) 
{ 
    const int *ia = (const int *)a;
    const int *ib = (const int *)b;
    return *ia  - *ib; 
}

int main() {
	pid_t a = 0, b = 0, c = 0;
	int s;
	int ap[2], bp[2], cp[2];
	pipe(ap);
	pipe(bp);
	pipe(cp);
	a = fork();
	if(a!=0)
		b = fork();
	if(b!=0)
		c = fork();
	if(c==0) {
		int p;
		if(a==0) p = ap[1];
		else if(b==0) p = bp[1];
		else p = cp[1];
		int ar[100];
		for(int i=0;i<100;i++)
			ar[i] = rand()%100+1;
		qsort(ar, 100, sizeof(int), cmp);
		for(int i=0;i<100;i++)
			write(p, &ar[i], sizeof(int));
		close(p);

	}
	else {
		int ar[300],r[3]={1,1,1},p[3]={ap[0],bp[0],cp[0]},t[3];
		for(int i=0;i<300;i++) {
			for(int j=0;j<3;j++)
				if(r[j]==1)
					read(p[j], &t[j], sizeof(int));
			if(t[0]<t[1]&&t[0]<t[2]) {
				ar[i]=t[0];
				r[0]=1;
			}
			else if(t[0]<t[2]) {
				ar[i]=t[1];
				r[1]=1;
			}
			else {
				ar[i]=t[2];
				r[2]=1;
			}
			cout<<ar[i]<<"\n";
		}
	}
}