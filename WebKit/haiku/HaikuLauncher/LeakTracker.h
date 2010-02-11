#ifndef LeakTracker_h
#define LeakTracker_h

// This code is from:
// http://www.flipcode.com/archives/How_To_Find_Memory_Leaks.shtml
// I have modified it some.

#ifndef NDEBUG

#if __GNUC__ > 2
    #include <string.h>
    using namespace std;
#endif

#include <stdio.h>
#include <vector>

typedef struct {
    int    address;
    size_t    size;
    char    file[64];
    int    line;
} ALLOC_INFO;

typedef vector<ALLOC_INFO*> AllocList;

AllocList *allocList;

void AddTrack(int addr, size_t asize, const char *fname, int lnum)
{
    ALLOC_INFO *info;

    if (!allocList)
        allocList = new(AllocList);

    info = new(ALLOC_INFO);
    info->address = addr;
    strncpy(info->file, fname, 63);
    info->line = lnum;
    info->size = asize;
    allocList->insert(allocList->begin(), info);
};

void RemoveTrack(int addr)
{
    AllocList::iterator i;

    if (!allocList)
        return;

    for (i = allocList->begin(); i < allocList->end(); i++) {
        if ((*i)->address == addr) {
            allocList->erase(i);
            break;
        }
    }
};

int unfreedCount()
{
    return allocList->size();
}

bool nothingUnfreed()
{
    return allocList->empty();
}

void OutputDebugString(char *str)
{
    printf(str);
}

void DumpUnfreed()
{
    AllocList::iterator i;
    int totalSize = 0;
    char buf[1024];

    if (!allocList)
        return;

    for (i = allocList->begin(); i < allocList->end(); i++) {
        sprintf(buf, "%-50s:\t\tLINE %d,\t\tADDRESS %d\t%d unfreed\n",
                (*i)->file, (*i)->line, (*i)->address, (int)(*i)->size);
        OutputDebugString(buf);
        totalSize += (*i)->size;
    }
    sprintf(buf, "-----------------------------------------------------------\n");
    OutputDebugString(buf);
    sprintf(buf, "Total Unfreed: %d bytes\n", totalSize);
    OutputDebugString(buf);
};

inline void *operator new(size_t size, const char *file, int line)
{
    void *ptr = (void *)malloc(size);
    AddTrack((int)ptr, size, file, line);
    return(ptr);
};

inline void operator delete(void *p) throw()
{
    RemoveTrack((int)p);
    free(p);
};

#define DEBUG_NEW new(__FILE__, __LINE__)

#else
#define DEBUG_NEW new
#endif


#define new DEBUG_NEW


#endif // LeakTracker_h

