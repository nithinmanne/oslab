#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

#define EXIT "quit"

int main()
{
    string comm;
    while(true) {
        cout<<">>> ";
        getline(cin, comm);
        if(comm==EXIT) break;
        int x = fork();
        chdir("dfs");
        int status;
        if(x==0) execlp("sh", "sh", "-c", comm.c_str(), nullptr);
        else wait(&status);
    }
}
