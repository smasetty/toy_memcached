#include <iostream>
#include <list>
#include <mutex>
#include "mempool.h"

#define MAX_PAGES   50
#define PAGE_SIZE   1048576

struct Page {
    void *data;
public:
    Page() {data = nullptr;};
};

std::list<struct Page *> pageFreeList;
std::list<struct Page *> pageAllocList;
/*
 * review if this lock is really needed later
 */
std::mutex mempoolLock;

static inline void PageListDestroy(std::list<struct Page* >& pList)
{
    std::list<struct Page*>::const_iterator it;

    for (it = pList.begin(); it != pList.end(); it++) {
        struct Page* page = *it;

        if (page->data)
            delete (char *)page->data;

        delete page;
    }
}

void MempoolDestroy()
{
    mempoolLock.lock();

    PageListDestroy(pageFreeList);
    PageListDestroy(pageAllocList);

    mempoolLock.unlock();
}

int MempoolInit(int numPages)
{
    int ret = 0;
    numPages = std::min(numPages, MAX_PAGES);

    if (numPages == 0)
        numPages = MAX_PAGES;

    mempoolLock.lock();

    for (int it = 0; it < numPages; it++) {

        struct Page* page = new(std::nothrow)(struct Page);
        if (!page) {
            ret = -1;
            break;
        }

        page->data = new(std::nothrow) char[PAGE_SIZE];
        if (!page->data) {
            delete page;
            ret = -1;
            break;
        }

        pageFreeList.push_back(page);
    }

    mempoolLock.unlock();

    if (ret)
        MempoolDestroy();

    return ret;
}

void* MempoolPageAlloc()
{
    if (!pageFreeList.size())
        return nullptr;

    mempoolLock.lock();

    std::list<Page *>::iterator it = pageFreeList.begin();
    pageFreeList.pop_front();

    struct Page* page = *it;

    pageAllocList.push_back(*it);

    mempoolLock.unlock();

    return page->data;
}

void MempoolPageFree(void* page)
{

}

int GetPageSize()
{
    return PAGE_SIZE;
}
