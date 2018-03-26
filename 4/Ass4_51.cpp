#include <iostream>

using namespace std;

#ifdef DEBUG
#define DEBUG true
#else
#define DEBUG false
#endif

int main() {
    if(DEBUG) cout<<"Hello, 4"<<endl;
}