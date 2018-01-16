#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/fcntl.h>

using namespace std;

#ifndef DEBUG
#define DEBUG false
#endif

#define W_MODE "Error! Invalid Mode.\n"
#define W_COMM "Error! Invalid Command.\n"

int run_int(string);

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
    if(DEBUG) cout<<"Parsed Redirect\n"<<"`"<<out[0]<<"`"<<out[1]<<"`\n";
    return 0;
}

vector<string> parse_pip(string comm) {
    vector<string> out;
    long loc;
    if(DEBUG) cout<<"Parsed Pipes\n";
    while((loc=comm.find('|'))!=-1) {
        out.push_back(comm.substr(0, (unsigned)loc));
        if(DEBUG) cout<<"`"<<comm.substr(0, (unsigned)loc)<<"`\n";
        comm = comm.substr((unsigned)loc+1, comm.length());
    }
    out.push_back(comm);
    return out;
}

int main()
{
    string mode, comm, comm_s[2];
    if(DEBUG) cout<<"Debug Build!\n";
    vector <string> comm_p;
    int status, x, in, out, pip[2];
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
        cout << "Enter Command: ";
        getline(cin, comm);
        switch(mode[0]) {
            case 'A':
                if(run_int(comm)) {
                    cout << W_COMM;
                    comm = W_COMM;
                }
                break;
            case 'B':
                x = fork();
                if(x==0) {
                    execlp("sh", "sh", "-c", comm.c_str(), nullptr);
                    exit(1);
                }
                else wait(&status);
                break;
            case 'C':
                if(parse_red(comm, '<', comm_s)) {
                    cout << W_COMM << "Must have one '<'\n";
                    comm = W_COMM;
                }
                x = fork();
                if(x==0) {
                    fclose(stdin);
                    dup(open(comm_s[1].c_str(), O_RDONLY));
                    execlp("sh", "sh", "-c", comm_s[0].c_str(), nullptr);
                    exit(1);
                }
                else wait(&status);
                break;
            case 'D':
                if(parse_red(comm, '>', comm_s)) {
                    cout << W_COMM << "Must have one '>'\n";
                    comm = W_COMM;
                }
                x = fork();
                if(x==0) {
                    fclose(stdout);
                    remove(comm_s[1].c_str());
                    dup(open(comm_s[1].c_str(), O_CREAT | O_WRONLY));
                    execlp("sh", "sh", "-c", comm_s[0].c_str(), nullptr);
                    exit(1);
                }
                else wait(&status);
                break;
            case 'E':
                x = (int)comm.find_last_not_of(' ');
                if(comm[x]!='&') {
                    cout << W_COMM << "Must end with &\n";
                    comm = W_COMM;
                }
                else {
                    comm = comm.substr(0, (unsigned)x);
                    if(DEBUG) cout << comm;
                    x = fork();
                    if (x == 0) {
                        execlp("sh", "sh", "-c", comm.c_str(), nullptr);
                        exit(1);
                    }
                }
                break;
            case 'F':
                comm_p = parse_pip(comm);
                in = -1;
                out = -1;
                for (const string &i : comm_p) {
                    if(DEBUG) cout << "'" << i << "'\n";
                    if(i!=comm_p.back()) {
                        pipe(pip);
                        out = pip[1];
                    }
                    x = fork();
                    if (x == 0) {
                        if(in!=-1) {
                            fclose(stdin);
                            dup(in);
                        }
                        if(out!=-1) {
                            fclose(stdout);
                            dup(out);
                        }
                        execlp("sh", "sh", "-c", comm.c_str(), nullptr);
                        exit(1);
                    }
                    in = pip[0];
                    out = -1;
                }
                for(int i=0;i<comm_p.size();i++) wait(&status);
                break;
            default:
                continue;
        }
    }
}

int run_int(string comm) {
    string func;
    vector<string> para;
    long loc = comm.find_first_not_of(' ');
    if(loc==-1) return 1;
    comm = comm.substr((unsigned)loc, comm.length());
    loc = comm.find_first_of(' ');
    if(loc==-1) loc = comm.length();
    func = comm.substr(0, (unsigned)loc);
    if(DEBUG) cout<<"Internal func\n`"<<func<<"`\n";
    comm = comm.substr((unsigned)loc, comm.length());
    if(DEBUG) cout<<"Remaining\n`"<<comm<<"`\n";
    loc = comm.find_first_not_of(' ');
    if(loc==-1) loc = comm.length();
    comm = comm.substr((unsigned)loc, comm.length());
    while(comm.length()>0) {
        loc = comm.find_first_of(' ');
        if(loc==-1) loc = comm.length();
        para.push_back(comm.substr(0, (unsigned)loc));
        if(DEBUG) cout<<"`"<<comm.substr(0, (unsigned)loc)<<"`\n";
        comm = comm.substr((unsigned)loc, comm.length());
        loc = comm.find_first_not_of(' ');
        if(loc==-1) loc = comm.length();
        comm = comm.substr((unsigned)loc, comm.length());
    }
    //TODO Write different functions for different func variable
    return 0;
}
