#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

#define EXIT "quit"

int main()
{
    string comm;
    while(true) {
        cout<<"A. Run an internal command\n";
        cout<<"B. Run an external command\n";
        cout<<"C. Run an external command by redirecting standard input from a file\n";
        cout<<"D. Run an external command by redirecting standard output to a file\n";
        cout<<"E. Run an external command in the background\n";
        cout<<"F. Run several external commands in the pipe mode\n";
        cout<<"G. Quit the shell\n";
        getline(cin, comm);
        if(comm==EXIT) break;
        int x = fork();
        chdir("dfs");
        int status;
        if(x==0) execlp("sh", "sh", "-c", comm.c_str(), nullptr);
        else wait(&status);
    }
}
