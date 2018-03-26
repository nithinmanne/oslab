#include "mrfs.h"

#define OWNER 1

mrfs::mrfs(): FS(nullptr), _key(0), init(false), sb(), key(_key), maxfdt(0) {}

mrfs::mrfs(const int& key): _key(key), init(true), sb(), key(_key), maxfdt(0) {
    int shmid = shmget(_key, sizeof sb.size, 0666);
    if(shmid==-1){
        std::cerr << "Shmget Failed for size" << std::endl;
        exit(1);
    }
    auto *sizep = (int*)shmat(shmid, nullptr, 0);
    int size = *sizep;
    shmdt(sizep);
    shmid = shmget(_key, static_cast<size_t>(size), 0666);
    if(shmid==-1){
        std::cerr << "Shmget Failed for FS" << std::endl;
        exit(1);
    }
    FS = (block*)shmat(shmid, nullptr, 0);
    sb = FS;
    inode = (indexnode*)(FS + sb.block_count);
    curdir = 0;
}

mrfs::mrfs(const mrfs& other): FS(other.FS), _key(other._key), init(other.init), sb(FS), inode(other.inode), curdir(other.curdir), key(_key), maxfdt(0) {}

mrfs& mrfs::operator=(const int& key) {
    if(init) {
      std::cerr << "Filesystem Already Initialized" << std::endl;
      return *this;
    }
    _key = key;
    init = true;
    int shmid = shmget(_key, sizeof sb.size, 0666);
    if(shmid==-1){
        std::cerr << "Shmget Failed for size" << std::endl;
        exit(1);
    }
    auto *sizep = (int*)shmat(shmid, nullptr, 0);
    int size = *sizep;
    shmdt(sizep);
    shmid = shmget(_key, static_cast<size_t>(size), 0666);
    if(shmid==-1){
        std::cerr << "Shmget Failed for size" << std::endl;
        exit(1);
    }
    FS = (block*)shmat(shmid, nullptr, 0);
    sb = FS;
    inode = (indexnode*)(FS + sb.block_count);
    curdir = 0;
    maxfdt = 0;
    return *this;
}



mrfs& mrfs::operator=(const mrfs& other) {
    if(init) {
      std::cerr << "Filesystem Already Initialized" << std::endl;
      return *this;
    }
    this->FS = other.FS;
    this->_key = other._key;
    this->init = other.init;
    this->inode = other.inode;
    this->curdir = other.curdir;
    sb = this->FS;
    maxfdt = 0;
    return *this;
}

mrfs::~mrfs() {
    shmdt(FS);
}

mrfs::superblock::superblock(): block_count(0),
                                size(block_count),
                                max_inodes(block_count),
                                used_inodes(block_count),
                                inodes(&block_count),
                                max_blocks(block_count),
                                used_blocks(block_count),
                                blocks(&block_count) {}

mrfs::superblock::superblock(block *FS): size(*(int*)FS),
                                        max_inodes(*(&size+1)),
                                        used_inodes(*(&max_inodes+1)),
                                        inodes(&used_inodes+1),
                                        max_blocks(*(inodes+max_inodes)),
                                        used_blocks(*(&max_blocks+1)),
                                        blocks(&used_blocks+1) {
    block_count = sizeof size + sizeof max_inodes + sizeof used_inodes +
                  max_inodes * sizeof *inodes + sizeof max_blocks + sizeof used_blocks +
                  max_blocks * sizeof *blocks;
    block_count = block_count/sizeof(block) + (block_count%sizeof(block)>0);
}

mrfs::superblock& mrfs::superblock::operator=(block* FS) {
    new (this) superblock(FS);
    return *this;
}

mrfs::blocklist::blocklist(): list(nullptr), blist(nullptr), numblocks(0) {}

mrfs::blocklist::blocklist(block* FS, indexnode& inode) {
    unsigned int num_blocks = inode.filesize/sizeof(block) + (inode.filesize%sizeof(block)>0);
    numblocks = num_blocks;
    list = new block*[num_blocks];
    blist = new int[num_blocks];
    unsigned int i=0;
    for(;i<8;i++) {
        if(i==num_blocks) return;
        list[i] = FS + inode.direct[i];
        blist[i] = inode.direct[i];
    }
    auto indirect = (int*)(FS + inode.indirect);
    for(unsigned int j=0;j<sizeof(block)/sizeof inode.indirect;i++,j++) {
        if(i==num_blocks) return;
        list[i] = FS + *indirect;
        blist[i] = *indirect;
        indirect++;
    }
    auto doubleindirect = (int*)(FS + inode.doubleindirect);
    for(unsigned int j=0;j<sizeof(block)/sizeof inode.doubleindirect;j++) {
        auto dindirect = (int*)(FS + *doubleindirect);
        for(unsigned int k=0;k<sizeof(block)/sizeof inode.indirect;i++,k++) {
            if(i==num_blocks) return;
            list[i] = FS + *dindirect;
            blist[i] = *dindirect;
            dindirect++;
        }
        doubleindirect++;
    }
}

mrfs::blocklist::~blocklist() {
    delete [] list;
}

mrfs::fdt::fdt(const fdt& other):  id(other.id), type(other.type), offset(other.offset), inode(other.inode) {};
mrfs::fdt::fdt(int id, int type, int offset, indexnode &inode):  id(id), type(type), offset(offset), inode(inode) {};
mrfs::fdt& mrfs::fdt::operator=(const fdt& other) {
    new (this) fdt(other);
    return *this;
}

int mrfs::create_myfs(int size){
    if(init){
        std::cerr << "Filesystem Already Initialized" << std::endl;
        return -1;
    }
    std::minstd_rand rand;
    rand.seed(static_cast<unsigned long>(time(nullptr)));
    _key = static_cast<int>(rand());
    int bsize = size*1024*1024;
    int shmid = shmget(_key, static_cast<size_t>(bsize), IPC_CREAT | 0666);
    if(shmid==-1){
        std::cerr << "Shmget Failed for FS" << std::endl;
        return -1;
    }
    FS = (block*)shmat(shmid, nullptr, 0);
    init = true;
    sb = FS;
    sb.size = bsize;
    sb.max_inodes = 1000;
    sb = FS;
    sb.used_inodes = 0;
    for (int i=0;i<sb.max_inodes;i++)
        sb.inodes[i] = 0;
    sb.max_blocks = bsize/sizeof(block);
    sb = FS;
    sb.used_blocks = 0;
    for (int i=0;i<sb.max_blocks;i++)
        sb.blocks[i] = 0;
    for (int i=0;i<sb.block_count;i++)
        sb.blocks[i+sb.used_blocks] = 1;
    sb.used_blocks += sb.block_count;
    inode = (indexnode*)(FS + sb.block_count);
    int inode_blocks = sizeof(indexnode)*sb.max_inodes;
    inode_blocks = inode_blocks/sizeof(block) + (inode_blocks%sizeof(block)>0);
    for (int i=0;i<inode_blocks;i++)
        sb.blocks[i+sb.used_blocks] = 1;
    sb.used_blocks += inode_blocks;
    auto rootinode = reqinode();
    sb.used_inodes++;
    sb.inodes[rootinode] = 1;
    auto& root = *(inode+rootinode);
    root.filetype = 1;
    root.filesize = sizeof(dirlist);
    root.lastModified = time(nullptr);
    root.lastRead = time(nullptr);
    root.acPermissions = 0666;
    root.owner = OWNER;
    root.direct[0] = reqblock();
    sb.used_blocks++;
    sb.blocks[root.direct[0]] = 1;
    auto& rootdot = *(dirlist*)(FS + root.direct[0]);
    strcpy(rootdot.name, ".");
    rootdot.inode = rootinode;
    curdir = rootdot.inode;
    return 0;
}

int mrfs::copy_pc2myfs(const char *source, const char *dest) {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    auto& curdir_inode = *(inode+curdir);
    int num_files;
    blocklist curdir_blocks(FS, curdir_inode);
    for (int i = 0; i<curdir_blocks.numblocks; i++){
        auto curdir_files = (dirlist*)(curdir_blocks[i]);
        num_files = sizeof(block)/sizeof(dirlist);
        if(i==curdir_blocks.numblocks-1) num_files = curdir_inode.filesize/sizeof(dirlist)-i*num_files;
        for(int j=0;j<num_files;j++)
            if(strcmp(curdir_files[j].name, dest)==0) {
                std::cerr << "Name Already Present" << std::endl;
                return -1;
            }
    }
    std::ifstream source_file;
    source_file.open(source, std::ios::in);
    if(source_file.fail()) {
        std::cerr << "File Open Failed" << std::endl;
        return -1;
    }
    std::string str((std::istreambuf_iterator<char>(source_file)), std::istreambuf_iterator<char>());
    auto fileinode = reqinode();
    if(fileinode==-1) {
        std::cerr << "Inode Allocation Failed" << std::endl;
        return -1;
    }
    sb.used_inodes++;
    sb.inodes[fileinode] = 1;
    auto& file = *(inode+fileinode);
    file.filetype = 0;
    file.filesize = 0;
    file.lastModified = time(nullptr);
    file.lastRead = time(nullptr);
    file.acPermissions = 0666;
    file.owner = OWNER;
    appendl(file, str.c_str(), str.size());
    auto *dirl = new dirlist;
    strcpy(dirl->name, dest);
    dirl->inode = fileinode;
    if(appendl(*(inode+curdir), (char*)dirl, sizeof(dirlist))==-1) {
        std::cerr << "Directory Full, Filesystem Corrupted!" << std::endl;
        delete dirl;
        return -1;
    }
    delete dirl;
    return 0;
}

int mrfs::copy_myfs2pc(const char *source, const char *dest) const {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    auto& curdir_inode = *(inode+curdir);
    int num_files;
    blocklist curdir_blocks(FS, curdir_inode);
    for (int i = 0; i<curdir_blocks.numblocks; i++){
        auto curdir_files = (dirlist*)(curdir_blocks[i]);
        num_files = sizeof(block)/sizeof(dirlist);
        if(i==curdir_blocks.numblocks-1) num_files = curdir_inode.filesize/sizeof(dirlist)-i*num_files;
        for(int j=0;j<num_files;j++)
            if(strcmp(curdir_files[j].name, source)==0) {
                auto& file_inode = *(inode+curdir_files[j].inode);
                if(file_inode.filetype==1) {
                    std::cerr << "Directory, Not File" << std::endl;
                    return -1;
                }
                std::ofstream dest_file;
                dest_file.open(dest, std::ios::out|std::ios::binary);
                if(dest_file.fail()) {
                    std::cerr << "Destination File Open Failed" << std::endl;
                    return -1;
                }
                blocklist file_blocks(FS, file_inode);
                for(int k = 0; k<file_blocks.numblocks;k++)
                    if(k==file_blocks.numblocks-1)
                        dest_file.write((char*) file_blocks[k], file_inode.filesize - k * sizeof(block));
                    else
                        dest_file.write((char*)file_blocks[k], sizeof(block));
                dest_file.close();
                return 0;
            }
    }
    std::cerr << "Source File Not Found" << std::endl;
    return -1;
}

int mrfs::rm_myfs(const char *filename) {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    auto& curdir_inode = *(inode+curdir);
    int num_files;
    blocklist curdir_blocks(FS, curdir_inode);
    for (int i = 0; i<curdir_blocks.numblocks; i++){
        auto curdir_files = (dirlist*)(curdir_blocks[i]);
        num_files = sizeof(block)/sizeof(dirlist);
        if(i==curdir_blocks.numblocks-1) num_files = curdir_inode.filesize/sizeof(dirlist)-i*num_files;
        for(int j=0;j<num_files;j++)
            if(strcmp(curdir_files[j].name, filename)==0) {
                auto& file_inode = *(inode+curdir_files[j].inode);
                if(file_inode.filetype==1) {
                    std::cerr << "Directory, Not File" << std::endl;
                    return -1;
                }
                deletel(file_inode, file_inode.filesize);
                sb.used_inodes--;
                sb.inodes[curdir_files[j].inode] = 0;
                strcpy(curdir_files[j].name, ((dirlist*)curdir_blocks[curdir_blocks.numblocks-1])[-1+curdir_inode.filesize/sizeof(dirlist)-(curdir_blocks.numblocks-1)*sizeof(block)/sizeof(dirlist)].name);
                curdir_files[j].inode = ((dirlist*)curdir_blocks[curdir_blocks.numblocks-1])[-1+curdir_inode.filesize/sizeof(dirlist)-(curdir_blocks.numblocks-1)*sizeof(block)/sizeof(dirlist)].inode;
                deletel(curdir_inode, sizeof(dirlist));
                return 0;
            }
    }
    std::cerr << "File Not Found" << std::endl;
    return -1;
}

int mrfs::showfile_myfs(const char *filename) const {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    auto& curdir_inode = *(inode+curdir);
    int num_files;
    blocklist curdir_blocks(FS, curdir_inode);
    for (int i = 0; i<curdir_blocks.numblocks; i++){
        auto curdir_files = (dirlist*)(curdir_blocks[i]);
        num_files = sizeof(block)/sizeof(dirlist);
        if(i==curdir_blocks.numblocks-1) num_files = curdir_inode.filesize/sizeof(dirlist)-i*num_files;
        for(int j=0;j<num_files;j++)
            if(strcmp(curdir_files[j].name, filename)==0) {
                auto& file_inode = *(inode+curdir_files[j].inode);
                if(file_inode.filetype==1) {
                    std::cerr << "Directory, Not File" << std::endl;
                    return -1;
                }
                blocklist file_blocks(FS, file_inode);
                for(int k = 0; k<file_blocks.numblocks;k++)
                    if(k==file_blocks.numblocks-1)
                        std::cout.write((char*)file_blocks[k], file_inode.filesize-k*sizeof(block));
                    else
                        std::cout.write((char*)file_blocks[k], sizeof(block));
                return 0;
            }
    }
    std::cerr << "File Not Found" << std::endl;
    return -1;
}
int mrfs::ls_myfs() const {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    auto& curdir_inode = *(inode+curdir);
    int num_files;
    blocklist curdir_blocks(FS, curdir_inode);
    std::cout << "Total Items: " << curdir_inode.filesize/sizeof(dirlist) << std::endl;
    for(int i=0;i<curdir_blocks.numblocks;i++) {
        auto curdir_files = (dirlist*)(curdir_blocks[i]);
        num_files = sizeof(block)/sizeof(dirlist);
        if(i==curdir_blocks.numblocks-1) num_files = curdir_inode.filesize/sizeof(dirlist)-i*num_files;
        for(int j=0;j<num_files;j++){
            auto& file_inode = *(inode+curdir_files[j].inode);
            std::cout << std::oct << file_inode.acPermissions << std::dec << " " << file_inode.owner << " " << file_inode.filesize << " " << curdir_files[j].name << std::endl;
        }
    }
    return 0;
}

int mrfs::mkdir_myfs(const char *dirname) {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    auto& curdir_inode = *(inode+curdir);
    int num_files;
    blocklist curdir_blocks(FS, curdir_inode);
    for (int i = 0; i<curdir_blocks.numblocks; i++){
        auto curdir_files = (dirlist*)(curdir_blocks[i]);
        num_files = sizeof(block)/sizeof(dirlist);
        if(i==curdir_blocks.numblocks-1) num_files = curdir_inode.filesize/sizeof(dirlist)-i*num_files;
        for(int j=0;j<num_files;j++)
            if(strcmp(curdir_files[j].name, dirname)==0) {
                std::cerr << "Name Already Present" << std::endl;
                return -1;
            }
    }
    auto dirinode = reqinode();
    if(dirinode==-1) {
        std::cerr << "Out Of inodes" << std::endl;
        return -1;
    }
    sb.used_inodes++;
    sb.inodes[dirinode] = 1;
    auto& dir = *(inode+dirinode);
    dir.filetype = 1;
    dir.filesize = 2*sizeof(dirlist);
    dir.lastModified = time(nullptr);
    dir.lastRead = time(nullptr);
    dir.acPermissions = 0666;
    dir.owner = OWNER;
    dir.direct[0] = reqblock();
    if(dir.direct[0]==-1) {
        std::cerr << "Out Of blocks, Filesystem Corrupted" << std::endl;
        return -1;
    }
    sb.used_blocks++;
    sb.blocks[dir.direct[0]] = 1;
    auto dirdot = (dirlist*)(FS + dir.direct[0]);
    strcpy(dirdot->name, ".");
    dirdot->inode = dirinode;
    dirdot++;
    strcpy(dirdot->name, "..");
    dirdot->inode = curdir;
    auto *dirl = new dirlist;
    strcpy(dirl->name, dirname);
    dirl->inode = dirinode;
    if(appendl(*(inode+curdir), (char*)dirl, sizeof(dirlist))==-1) {
        std::cerr << "Directory Full, Filesystem Corrupted!" << std::endl;
        delete dirl;
        return -1;
    }
    delete dirl;
    return 0;
}

int mrfs::chdir_myfs(const char *dirname) {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    auto& curdir_inode = *(inode+curdir);
    int num_files;
    blocklist curdir_blocks(FS, curdir_inode);
    for (int i = 0; i<curdir_blocks.numblocks; i++){
        auto curdir_files = (dirlist*)(curdir_blocks[i]);
        num_files = sizeof(block)/sizeof(dirlist);
        if(i==curdir_blocks.numblocks-1) num_files = curdir_inode.filesize/sizeof(dirlist)-i*num_files;
        for(int j=0;j<num_files;j++)
            if(strcmp(curdir_files[j].name, dirname)==0) {
                auto& file_inode = *(inode+curdir_files[j].inode);
                if(file_inode.filetype==0) {
                    std::cerr << "File, Not Directory" << std::endl;
                    return -1;
                }
                curdir = curdir_files[j].inode;
                return 0;
            }
    }
    std::cerr << "Directory Not Found" << std::endl;
    return -1;
}

int mrfs::rmdir_myfs(const char *dirname) {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    if(strcmp(".", dirname)==0) {
        std::cerr << "Cannot delete ." << std::endl;
        return -1;
    }
    if(strcmp("..", dirname)==0) {
        std::cerr << "Cannot delete .." << std::endl;
        return -1;
    }
    auto& curdir_inode = *(inode+curdir);
    int num_files;
    blocklist curdir_blocks(FS, curdir_inode);
    for (int i = 0; i<curdir_blocks.numblocks; i++){
        auto curdir_files = (dirlist*)(curdir_blocks[i]);
        num_files = sizeof(block)/sizeof(dirlist);
        if(i==curdir_blocks.numblocks-1) num_files = curdir_inode.filesize/sizeof(dirlist)-i*num_files;
        for(int j=0;j<num_files;j++)
            if(strcmp(curdir_files[j].name, dirname)==0) {
                auto& dir_inode = *(inode+curdir_files[j].inode);
                if(dir_inode.filetype==0) {
                    std::cerr << "File, Not Directory" << std::endl;
                    return -1;
                }
                if(dir_inode.filesize>2*sizeof(dirlist)) {
                    std::cerr << "Directory Not Empty" << std::endl;
                    return -1;
                }
                deletel(dir_inode, dir_inode.filesize);
                sb.used_inodes--;
                sb.inodes[curdir_files[j].inode] = 0;
                strcpy(curdir_files[j].name, ((dirlist*)curdir_blocks[curdir_blocks.numblocks-1])[-1+curdir_inode.filesize/sizeof(dirlist)-(curdir_blocks.numblocks-1)*sizeof(block)/sizeof(dirlist)].name);
                curdir_files[j].inode = ((dirlist*)curdir_blocks[curdir_blocks.numblocks-1])[-1+curdir_inode.filesize/sizeof(dirlist)-(curdir_blocks.numblocks-1)*sizeof(block)/sizeof(dirlist)].inode;
                deletel(curdir_inode, sizeof(dirlist));
                return 0;
            }
    }
    std::cerr << "Directory Not Found" << std::endl;
    return -1;
}

int mrfs::open_myfs(const char *filename, char mode) {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    auto& curdir_inode = *(inode+curdir);
    int num_files;
    blocklist curdir_blocks(FS, curdir_inode);
    for (int i = 0; i<curdir_blocks.numblocks; i++){
        auto curdir_files = (dirlist*)(curdir_blocks[i]);
        num_files = sizeof(block)/sizeof(dirlist);
        if(i==curdir_blocks.numblocks-1) num_files = curdir_inode.filesize/sizeof(dirlist)-i*num_files;
        for(int j=0;j<num_files;j++)
            if(strcmp(curdir_files[j].name, filename)==0) {
                auto& file_inode = *(inode+curdir_files[j].inode);
                if(file_inode.filetype==1) {
                    std::cerr << "Directory, Not File" << std::endl;
                    return -1;
                }
                if(mode=='r') {
                    fdt nfdt{maxfdt, 0, 0, file_inode};
                    fdtlist.push_back(nfdt);
                }
                else if(mode=='w') {
                    deletel(file_inode, file_inode.filesize);
                    fdt nfdt{maxfdt, 1, 0, file_inode};
                    fdtlist.push_back(nfdt);
                }
                else {
                    std::cerr << "Invalid Mode" << std::endl;
                    return -1;
                }
                maxfdt++;
                return maxfdt-1;
            }
    }
    if(mode=='w') {
        auto fileinode = reqinode();
        if(fileinode==-1) {
            std::cerr << "Inode Allocation Failed" << std::endl;
            return -1;
        }
        sb.used_inodes++;
        sb.inodes[fileinode] = 1;
        auto& file = *(inode+fileinode);
        file.filetype = 0;
        file.filesize = 0;
        file.lastModified = time(nullptr);
        file.lastRead = time(nullptr);
        file.acPermissions = 0666;
        file.owner = OWNER;
        auto *dirl = new dirlist;
        strcpy(dirl->name, filename);
        dirl->inode = fileinode;
        if(appendl(*(inode+curdir), (char*)dirl, sizeof(dirlist))==-1) {
            std::cerr << "Directory Full, Filesystem Corrupted!" << std::endl;
            delete dirl;
            return -1;
        }
        delete dirl;
        fdt nfdt{maxfdt, 1, 0, file};
        fdtlist.push_back(nfdt);
        maxfdt++;
        return maxfdt-1;
    }
    std::cerr << "File Not Found" << std::endl;
    return -1;
}

int mrfs::close_myfs(int fd) {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    for(int i=0;i<fdtlist.size();i++)
        if(fdtlist[i].id==fd) {
            fdtlist.erase(fdtlist.begin()+i);
            return 0;
        }
    std::cerr << "FD not found" << std::endl;
    return -1;
}

int mrfs::read_myfs(int fd, int nbytes, char *buff) {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    for(auto &i: fdtlist) {
        if(i.id==fd) {
            if(i.type==1) {
                std::cerr << "FD is in write mode" << std::endl;
                return -1;
            }
            blocklist bl(FS, i.inode);
            if(i.inode.filesize==i.offset) return 0;
            if(nbytes>i.inode.filesize-i.offset)
                nbytes = i.inode.filesize-i.offset;
            auto left=(size_t)nbytes;
            int bl2rd = i.offset/sizeof(block);
            if(i.offset%sizeof(block)!=0) {
                auto to_read = (sizeof(block)-i.offset%sizeof(block))>nbytes?(size_t)nbytes:(sizeof(block)-i.offset%sizeof(block));
                memcpy(buff, (char*)(bl[bl2rd])+i.offset%sizeof(block), to_read);
                buff += to_read;
                left -= to_read;
                bl2rd++;
            }
            while(left>0) {
                auto to_read = left>sizeof(block)?sizeof(block):left;
                memcpy(buff, bl[bl2rd], to_read);
                buff += to_read;
                left -= to_read;
                bl2rd++;
            }
            i.offset += nbytes;
            return nbytes;
        }
    }
    std::cerr << "FD not found" << std::endl;
    return -1;
}

int mrfs::write_myfs(int fd, int nbytes, const char *buff) {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    for(auto &i: fdtlist) {
        if(i.id==fd) {
            if(i.type==0) {
                std::cerr << "FD is in read mode" << std::endl;
                return -1;
            }
            appendl(i.inode, buff, (size_t)nbytes);
            return 0;
        }
    }
    std::cerr << "FD not found" << std::endl;
    return -1;
}

int mrfs::eof_myfs(int fd) {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    for(auto &i: fdtlist) {
        if(i.id==fd) {
            if(i.offset==i.inode.filesize) return 1;
            else return 0;
        }
    }
    std::cerr << "FD not found" << std::endl;
    return -1;
}

int mrfs::dump_myfs (const char *dumpfile) const {
    if(!init) {
      std::cerr << "Filesystem Not Initialized" << std::endl;
      return -1;
    }
    std::ofstream dump_file;
    dump_file.open(dumpfile, std::ios::out|std::ios::binary);
    if(dump_file.fail()) {
        std::cerr << "File Open Failed" << std::endl;
        return -1;
    }
    dump_file.write((char*)FS, sb.size);
    dump_file.close();
    return 0;
}

int mrfs::restore_myfs (const char *dumpfile) {
    if(init) {
      std::cerr << "Filesystem Already Initialized" << std::endl;
      return -1;
    }
    std::ifstream dump_file;
    dump_file.open(dumpfile, std::ios::in|std::ios::binary);
    if(dump_file.fail()) {
        std::cerr << "File Open Failed" << std::endl;
        return -1;
    }
    int size;
    dump_file.read((char*)&size, sizeof size);
    std::minstd_rand rand;
    rand.seed(static_cast<unsigned long>(time(nullptr)));
    _key = static_cast<int>(rand());
    int shmid = shmget(_key, static_cast<size_t>(size), IPC_CREAT | 0666);
    if(shmid==-1){
        std::cerr << "Shmget Failed for FS" << std::endl;
        return -1;
    }
    FS = (block*)shmat(shmid, nullptr, 0);
    dump_file.seekg(0, std::ios::beg);
    dump_file.read((char*)FS, size);
    if(dump_file.fail()) {
        std::cerr << "Invalid Dump File for size " << size << std::endl;
        return -1;
    }
    init = true;
    sb = FS;
    inode = (indexnode*)(FS + sb.block_count);
    curdir = 0;
    dump_file.close();
    return 0;
}

int mrfs::status_myfs() const {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    std::cout << "Total Size of File System: " << sb.size << std::endl;
    std::cout << "Occupied Size of File System: " << sb.used_blocks*sizeof(block) << std::endl;
    std::cout << "Free Size of File System: " << sb.size-sb.used_blocks*sizeof(block) << std::endl;
    std::cout << "Total Number of Files: " << sb.used_inodes << std::endl;
    return 0;
}

int mrfs::chmod_myfs(const char *name, int mode) {
    if(!init) {
        std::cerr << "Filesystem Not Initialized" << std::endl;
        return -1;
    }
    auto& curdir_inode = *(inode+curdir);
    int num_files;
    blocklist curdir_blocks(FS, curdir_inode);
    for (int i = 0; i<curdir_blocks.numblocks; i++){
        auto curdir_files = (dirlist*)(curdir_blocks[i]);
        num_files = sizeof(block)/sizeof(dirlist);
        if(i==curdir_blocks.numblocks-1) num_files = curdir_inode.filesize/sizeof(dirlist)-i*num_files;
        for(int j=0;j<num_files;j++)
            if(strcmp(curdir_files[j].name, name)==0) {
                auto& file_inode = *(inode+curdir_files[j].inode);
                file_inode.acPermissions = mode;
                file_inode.acPermissions;
                return 0;
            }
    }
    std::cerr << "File/Directory Not Found" << std::endl;
    return -1;
}

int mrfs::appendl(indexnode &inode, const char *data, size_t size) {
    auto old_size = inode.filesize;
    blocklist bl(FS, inode);
    inode.filesize += size;
    if(old_size%sizeof(block)!=0) {
        auto to_copy = (size>(sizeof(block)-old_size%sizeof(block)))?(sizeof(block)-old_size%sizeof(block)):size;
        memcpy((char*)(bl[bl.numblocks-1])+old_size%sizeof(block), data, to_copy);
        size -= to_copy;
    }
    int blknum = bl.numblocks+1;
    block *blk;
    while(size>0) {
        if(blknum<=8) {
            inode.direct[blknum-1] = reqblock();
            if(inode.direct[blknum-1]==-1) {
                std::cerr << "Out of Blocks, Filesystem Corrupted" << std::endl;
                return -1;
            }
            blk = FS + inode.direct[blknum-1];

        }
        else if(blknum<=8+sizeof(block)/sizeof inode.indirect) {
            if(blknum==8+1) {
                inode.indirect = reqblock();
                if(inode.indirect==-1) {
                    std::cerr << "Out of Blocks, Filesystem Corrupted" << std::endl;
                    return -1;
                }
            }
            auto indirect = (int*)(FS + inode.indirect);
            indirect[blknum-(8+1)] = reqblock();
            if(indirect[blknum-(8+1)]==-1) {
                std::cerr << "Out of Blocks, Filesystem Corrupted" << std::endl;
                return -1;
            }
            blk = FS + indirect[blknum-(8+1)];
        }
        else {
            if(blknum==8+sizeof(block)/sizeof inode.indirect+1) {
                inode.doubleindirect = reqblock();
                if(inode.doubleindirect==-1) {
                    std::cerr << "Out of Blocks, Filesystem Corrupted" << std::endl;
                    return -1;
                }
            }
            if((blknum-8)%(sizeof(block)/sizeof inode.indirect)==1) {
                auto dindirect = (int*)(FS + inode.doubleindirect);
                dindirect[(blknum-(8+sizeof(block)/sizeof inode.indirect))/(sizeof(block)/sizeof inode.indirect)] = reqblock();
                if(dindirect[(blknum-(8+sizeof(block)/sizeof inode.indirect))/(sizeof(block)/sizeof inode.indirect)]==-1) {
                    std::cerr << "Out of Blocks, Filesystem Corrupted" << std::endl;
                    return -1;
                }
            }
            auto dindirect = (int*)(FS + inode.doubleindirect);
            auto indirect = (int*)(FS + dindirect[(blknum-(8+sizeof(block)/sizeof inode.indirect))/(sizeof(block)/sizeof inode.indirect)]);
            indirect[(blknum-(1+8+sizeof(block)/sizeof inode.indirect))%sizeof(block)/sizeof inode.indirect] = reqblock();
            if(indirect[(blknum-(1+8+sizeof(block)/sizeof inode.indirect))%sizeof(block)/sizeof inode.indirect]==-1) {
                std::cerr << "Out of Blocks, Filesystem Corrupted" << std::endl;
                return -1;
            }
            blk = FS + indirect[(blknum-(1+8+sizeof(block)/sizeof inode.indirect))%sizeof(block)/sizeof inode.indirect];
        }
        auto to_copy = size>sizeof(block)?sizeof(block):size;
        memcpy(blk, data, to_copy);
        size -= to_copy;
        data += to_copy;
        blknum++;
    }
    return 0;
}

int mrfs::deletel(indexnode &inode, int size) {
    inode.filesize -= size;
    int old_blocks = (inode.filesize+size)/sizeof(block);
    int new_blocks = inode.filesize/sizeof(block);
    if(old_blocks==new_blocks) return 0;
    int to_free = old_blocks - new_blocks;
    blocklist bl(FS, inode);
    while(to_free>0) {
        sb.used_blocks--;
        sb.blocks[bl.blist[old_blocks-1]] = 0;
        if(old_blocks==8+1) {
            sb.used_blocks--;
            sb.blocks[inode.indirect] = 0;
        }
        else if(old_blocks==8+sizeof(block)/sizeof inode.indirect+1) {
            sb.used_blocks -= 2;
            auto indirect = (int*)(FS+inode.doubleindirect);
            sb.blocks[*indirect] = 0;
            sb.blocks[inode.doubleindirect] = 0;
        }
        else if((old_blocks-8)%(sizeof(block)/sizeof inode.indirect)==1) {
            sb.used_blocks--;
            auto indirect = (int*)(FS+inode.doubleindirect);
            sb.blocks[*(indirect+(old_blocks-(8+sizeof(block)/sizeof inode.indirect)))] = 0;
            sb.blocks[inode.doubleindirect] = 0;
        }
        old_blocks--;
        to_free = old_blocks - new_blocks;
    }
    return 0;
}




//
