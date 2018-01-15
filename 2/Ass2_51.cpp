#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/fcntl.h>

using namespace std;

#define EXIT "quit"
#define W_MODE "Error! Invalid Mode!\n"
#define W_COMM "Error! Invalid Command!\n"

bool invalid(const string &mode) {
    if(mode.length()!=1) return true;
    if(mode[0]=='A') return false;
    if(mode[0]=='B') return false;
    if(mode[0]=='C') return false;
    if(mode[0]=='D') return false;
    if(mode[0]=='E') return false;
    if(mode[0]=='F') return false;
    return mode[0] != 'G';
}

int parse_red(const string &comm, char sep, string *out) {
    long loc = comm.find(sep);
    if(loc==-1) return 1;
    out[0] = comm.substr(0, (unsigned)loc);
    out[1] = comm.substr((unsigned)loc+1, comm.length());
    unsigned long fns = out[1].find_first_not_of(' ');
    out[1] = out[1].substr(fns, out[1].length());
    return 0;
}

int main()
{
    string mode, comm, comm_s[2];
    int status, x;
    while(true) {
        if(comm!=W_MODE && comm!=W_COMM) {
            cout << "A. Run an internal command\n";
            cout << "B. Run an external command\n";
            cout << "C. Run an external command by redirecting standard input from a file\n";
            cout << "D. Run an external command by redirecting standard output to a file\n";
            cout << "E. Run an external command in the background\n";
            cout << "F. Run several external commands in the pipe mode\n";
            cout << "G. Quit the shell\n";
        }
        if(comm!=W_COMM) {
            cout << "Enter Mode: ";
            getline(cin, mode);
            if (invalid(mode)) {
                cout << W_MODE;
                comm = W_MODE;
                continue;
            }
            if (mode[0] == 'G')
                break;
        }
        cout<<"Enter Command: ";
        getline(cin, comm);
        switch(mode[0]) {
            case 'A':
                break;
            case 'B':
                x = fork();
                if(x==0) execlp("sh", "sh", "-c", comm.c_str(), nullptr);
                else wait(&status);
                break;
            case 'C':
                parse_red(comm, '<', comm_s);
                x = fork();
                if(x==0) {
                    fclose(stdin);
                    dup(open(comm_s[1].c_str(), O_RDONLY));
                    execlp("sh", "sh", "-c", comm_s[0].c_str(), nullptr);
                }
                else wait(&status);
                break;
            case 'D':
                parse_red(comm, '>', comm_s);
                x = fork();
                if(x==0) {
                    fclose(stdout);
                    remove(comm_s[1].c_str());
                    dup(open(comm_s[1].c_str(), O_CREAT | O_WRONLY));
                    execlp("sh", "sh", "-c", comm_s[0].c_str(), nullptr);
                }
                else wait(&status);
                break;
            case 'E':
                x = (int)comm.find_last_not_of(' ');
                if(comm[x]!='&') {
                    cout<<W_COMM<<"Must end with &\n";
                    comm = W_COMM;
                }
                else {
                    comm = comm.substr(0, (unsigned)x);
                    cout<<comm;
                    x = fork();
                    if (x == 0) execlp("sh", "sh", "-c", comm.c_str(), nullptr);
                }
                break;
            case 'F':
                break;
            default:
                continue;
        }
    }
}
