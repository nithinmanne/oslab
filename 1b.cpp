#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;

int main() 
{
	char * com;
	char * exit;
	exit = "quit";
	while(1) {
	    cout << ">>";
	    gets(com);
	    if(strcmp(com, exit)==0)
	    	break;
	    int x = fork();
	    int status;
	    if(x==0)
	    	execlp(com, com);
	    else
	    	wait(&status);

	}
}