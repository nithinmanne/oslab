#include <iostream>
#include <fstream>
#include <sstream>
#include <bitset>
#include <ctime>

using namespace std;

#ifdef DEBUG
#define DEBUG true
#else
#define DEBUG false
#endif

enum class pageReplacement {
    FIFO,
    Random,
    LRU,
    NRU,
    secondChance
};
const auto pageReplacementUsed = pageReplacement::NRU;

class entry {
private:
    uint32_t mem;
    template<int bit>
    bool getbit() { return (bool)((mem & ( 1 << bit )) >> bit); }
    template<int bit>
    void setbit(bool nbit) {
        if(nbit)
            mem |= (1u << bit);
        else
            mem &= ~(1u << bit);
    }
public:
    entry(): mem(0) {}
    uint8_t frameNum() { return *(uint8_t*)(&mem); }
    void frameNum(uint8_t frameNum) { *(uint8_t*)(&mem) = frameNum; }
    bool valid() { return getbit<29>(); }
    void valid(bool valid) { setbit<29>(valid); }
    bool refer() { return getbit<30>(); }
    void refer(bool refer) { setbit<30>(refer); }
    bool modif() { return getbit<31>(); }
    void modif(bool modif) { setbit<31>(modif); }
};
static_assert(sizeof(entry)==4, "Entry not equal to 32 bits.");

long mini(const int *ar, long n) {
    if(n==0) return -1;
    long mini = 0;
    for(long i=1;i<n;i++)
        if(ar[mini]>ar[i])
            mini = i;
    return mini;
}

int main(int argc, char *argv[]) {
    string fName;
    long nPageFrames;
    if(argc==1) {
        cout<<"Enter File Name: ";
        cin>>fName;
        cout<<"Enter Number of Page Frames: ";
        cin>>nPageFrames;
    }
    else if(argc==2) {
        fName = argv[1];
        cout<<"Enter Number of Page Frames: ";
        cin>>nPageFrames;
    }
    else {
        fName = argv[1];
        nPageFrames = strtol(argv[2], nullptr, 0);
    }
    auto pageTable = new entry[nPageFrames];
    auto useCount = new int[nPageFrames];
    for(long i=0;i<nPageFrames;i++)
        useCount[i] = 0;
    ifstream inFile(fName);
    int curLine = 1;
    long totCycles = 0;
    int numPageFault = 0;
    int numPageTransfer = 0;
    long fifoIndex = 0;
    for(string line; getline(inFile, line); )
    {
        if(line.find_first_not_of(' ')!=string::npos)
            line = line.substr(line.find_first_not_of(' '));
        else
            continue;
        if(line[0]=='#') continue;
        bool rw;
        uint8_t pNum;
        int tmp;
        stringstream lineStream(line);
        lineStream>>rw>>tmp;
        pNum = (uint8_t)tmp;
        long aPage;
        bool found = false;
        //Finding if Page is already present
        for(aPage=0;aPage<nPageFrames;aPage++)
            if(pageTable[aPage].valid())
                if(pageTable[aPage].frameNum()==pNum) {
                    found = true;
                    break;
                }
        //Finding Free Page if not found
        if(!found) {
            numPageFault++;
            fifoIndex++;
            for(aPage=0;aPage<nPageFrames;aPage++)
                if(!pageTable[aPage].valid()) {
                    found = true;
                    pageTable[aPage].frameNum(pNum);
                    pageTable[aPage].valid(true);
                    pageTable[aPage].refer(false);
                    pageTable[aPage].modif(false);
                    cout<<curLine<<":\tIN   \t"<<(int)pNum<<"\t"<<aPage<<endl;
                    totCycles += 3000;
                    numPageTransfer++;
                    cout<<curLine<<":\tMAP  \t"<<(int)pNum<<"\t"<<aPage<<endl;
                    totCycles += 250;
                    break;
                }
        }
        //Page Replacement if no Free Page found
        if(!found) {
            long pageToReplace = 0;
            switch(pageReplacementUsed) {
                case pageReplacement::FIFO:
                    pageToReplace = fifoIndex;
                    break;
                case pageReplacement::Random:
                    pageToReplace = rand()%nPageFrames; //NOLINT
                    break;
                case pageReplacement::LRU:
                    pageToReplace = mini(useCount, nPageFrames);
                    break;
                case pageReplacement::NRU:
                    pageToReplace = (fifoIndex+1)&nPageFrames;
                    while(true) {
                        if(pageToReplace==fifoIndex) break;
                        if(pageTable[pageToReplace].refer())
                            pageToReplace++;
                        else
                            break;
                    }
                    break;
                case pageReplacement::secondChance:
                    pageToReplace = fifoIndex;
                    while(true) {
                        if(pageTable[pageToReplace].refer()) {
                            pageTable[pageToReplace].refer(false);
                            pageToReplace++;
                            pageToReplace = pageToReplace%nPageFrames;
                        }
                        else
                            break;
                    }
                    break;
            }
            fifoIndex++;
            aPage = pageToReplace;
            cout<<curLine<<":\tUNMAP\t"<<(int)pageTable[aPage].frameNum()<<"\t"<<aPage<<endl;
            totCycles += 250;
            if(pageTable[aPage].modif()) {
                cout<<curLine<<":\tOUT  \t"<<(int)pageTable[aPage].frameNum()<<"\t"<<aPage<<endl;
                totCycles += 3000;
                numPageTransfer++;
            }
            pageTable[aPage].frameNum(pNum);
            pageTable[aPage].valid(true);
            pageTable[aPage].refer(false);
            pageTable[aPage].modif(false);
            cout<<curLine<<":\tIN   \t"<<(int)pNum<<"\t"<<aPage<<endl;
            totCycles += 3000;
            numPageTransfer++;
            cout<<curLine<<":\tMAP  \t"<<(int)pNum<<"\t"<<aPage<<endl;
            totCycles += 250;
        }
        useCount[aPage] = (int)time(nullptr);
        pageTable[aPage].refer(true);
        if(rw==1)
            pageTable[aPage].modif(true);
        totCycles++;
        curLine++;
        if(pageReplacementUsed==pageReplacement::NRU && curLine%10==0)
            for(long i=0;i<nPageFrames;i++) {
                pageTable[i].refer(false);
            }
        fifoIndex = fifoIndex%nPageFrames;
    }
    cout<<"Number of Page Faults: "<<numPageFault<<endl;
    cout<<"Number of Page Transfers: "<<numPageTransfer<<endl;
    cout<<"Total Cycles: "<<totCycles<<endl;
    inFile.close();
    delete[] pageTable;
}