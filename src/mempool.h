#ifndef MEMPOOL
#define MEMPOOL

int MempoolInit(int numPages);

void MempoolDestroy();

void* MempoolPageAlloc();

void MempoolPageFree(void* page);

int GetPageSize();

#endif /* ifndef MEMPOOL */
