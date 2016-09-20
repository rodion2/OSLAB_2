#include "mmemory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define TRUE			1
#define FALSE			0
#define ALLOCSIZE		128				/* îáúåì èìåþùåéñÿ ïàìÿòè (128 áàéò) */
#define NULL_SYMBOL		'\0'			/* ñèìâîë èíèöèàëèçàöèè ïàìÿòè */
#define SWAPING_NAME	"swap.dat"		/* ôàéë ïîäêà÷êè */

/* Êîäû âîçâðàòà */
const int SUCCESSFUL_CODE				=  0;
const int INCORRECT_PARAMETERS_ERROR	= -1;
const int NOT_ENOUGH_MEMORY_ERROR		= -2;
const int OUT_OF_RANGE_ERROR			= -2;
const int UNKNOWN_ERROR					=  1;

/* Áëîê íåïðåðûâíîé ïàìÿòè (çàíÿòîé èëè ñâîáîäíûé) */
struct block {
    struct block*	pNext;			// ñëåäóþùèé ñâîáîäíûé èëè çàíÿòûé áëîê
    unsigned int	szBlock;		// ðàçìåð áëîêà
    unsigned int	offsetBlock;	// ñìåùåíèå áëîêà îòíîñèòåëüíî ñòðàíèöû
};

/* Èíôîðìàöèÿ î ñòðàíèöå â òàáëèöå ñòðàíèö */
struct pageInfo {
    unsigned long	offsetPage;		// ñìåùåíèå ñòðàíèöû îò íà÷àëà ïàìÿòè èëè ôàéëà
    char			isUse;			// ôëàã, îïèñûâàþùèé íàõîäèòñÿ ëè ñòðàíèöà â ïàìÿòè èëè íà äèñêå
};

/* Çàãîëîâîê ñòðàíèöû */
typedef struct page {
    struct block*	pFirstFree;			// ïåðâûé ñâîáîäíûé áëîê
    struct block*	pFirstUse;			// ïåðâûé çàíÿòûé áëîê
    unsigned int	maxSizeFreeBlock;	// ìàêñ. ðàçìåð ñâîäíîãî áëîêà
    struct pageInfo	pInfo;				// èíôîðìàöèÿ î ñòðàíèöå
    unsigned int	activityFactor;		// êîýôôèöèåíò àêòèâíîñòè
} page;

/* Òàáëèöà ñòðàíèö */
struct page *pageTable;

/* Êîëè÷åñòâî ñòðàíèö (â ïàìÿòè è íà äèñêå) */
int numberOfPages = 0;

/* Ðàçìåð ñòðàíèöû */
int sizePage = 0;

/* Ôèçè÷åñêàÿ ïàìÿòü */
VA allocbuf;

void ___print_memory();
int initPageTable();
void addToUsedBlocks(struct block *_block, struct page *_page);
void addToFreeBlocks(struct block *_block, struct page *_page);
int mallocInPage(VA* ptr, size_t szBlock, int pageNum);
void nullMemory();
unsigned int getMaxSizeFreeBlock(struct page _page);
struct page* getLeastActivePageInMemory();
int swap(struct page *memoryPage, struct page *swopPage);
int readf(unsigned long offsetPage, VA readPage);
int writef(unsigned long offsetPage, VA writtenPage);
VA convertVirtualtoPhysicalAddress(VA ptr, int *offsetPage, int *offsetBlock);
struct block* findBlock(int offsetPage, int offsetBlock);
struct block* findParentBlock(int offsetPage, int offsetBlock);

int _malloc (VA* ptr, size_t szBlock)
{
    int i;
    struct page *reservePageInSwap = NULL;
    int reservePageNum;

    if (!ptr || szBlock > (unsigned int) sizePage)
    {
        return INCORRECT_PARAMETERS_ERROR;
    }

    // îáõîäèì âñå ñòðàíèöû
    for (i = 0; i < numberOfPages; i++)
    {
        // åñëè â ñòðàíèöå åñòü áëîê äîñòàòî÷íîé ñâîáîäíîé ïàìÿòè
        if (pageTable[i].maxSizeFreeBlock >= szBlock)
        {
            // åñëè ýòà ñòðàíèöà â ïàìÿòè, òî íàì ïîâåçëî
            if (pageTable[i].pInfo.isUse)
            {
                return mallocInPage(ptr, szBlock, i);
            }
            else if (!reservePageInSwap)
            {
                // åñëè ýòà ñòðàíèöà â ñâîïå, òî çàïîìèíàåì å¸
                // íà ñëó÷àé, åñëè â ïàìÿòè ïîäõîäÿùåé ñòðàíèöû íå áóäåò íàéäåíî
                reservePageInSwap = &pageTable[i];
                reservePageNum = i;
            }
        }
    }
    // åñëè ñâîáîäíàÿ ñòðàíèöà íàøëàñü òîëüêî â ñâîïå
    if (reservePageInSwap)
    {
        int __errno = swap(getLeastActivePageInMemory(), reservePageInSwap);
        return mallocInPage(ptr, szBlock, reservePageNum);
    }
    return NOT_ENOUGH_MEMORY_ERROR;
}

int _free (VA ptr)
{
    VA raddr;
    int indexPage, offsetBlock;
    struct block *freeBlock, *parentBlock;
    if (!ptr)
    {
        return INCORRECT_PARAMETERS_ERROR;
    }
    raddr = convertVirtualtoPhysicalAddress(ptr, &indexPage, &offsetBlock);
    freeBlock = findBlock(indexPage, offsetBlock);
    parentBlock = findParentBlock(indexPage, offsetBlock);
    if (parentBlock)
    {
        parentBlock -> pNext = freeBlock -> pNext;
    }
    else
    {
        pageTable[indexPage].pFirstUse = freeBlock -> pNext;
    }
    addToFreeBlocks(freeBlock, &pageTable[indexPage]);
    return SUCCESSFUL_CODE;
    //return UNKNOWN_ERROR;
}

int _read (VA ptr, void* pBuffer, size_t szBuffer)
{
    VA raddr, vaBuffer;
    int indexPage, offsetBlock;
    unsigned int j;
    struct block* readBlock;
    if (!ptr || !pBuffer || (szBuffer > (unsigned int) sizePage))
    {
        return INCORRECT_PARAMETERS_ERROR;
    }
    raddr = convertVirtualtoPhysicalAddress(ptr, &indexPage, &offsetBlock);
    // åñëè íå ïîëó÷èëè àäðåñ, òî íóæíûé áëîê â ñâîïå
    // âûïîëíÿåì ñòðàíè÷íîå ïðåðûâàíèå
    if (!raddr)
    {
        int __errno = swap(getLeastActivePageInMemory(), &pageTable[indexPage]);
        raddr = convertVirtualtoPhysicalAddress(ptr, &indexPage, &offsetBlock);
    }
    //___print_memory();
    readBlock = findBlock(indexPage, offsetBlock);
    if (szBuffer <= readBlock -> szBlock)
    {
        VA raddr2;
        vaBuffer = (VA) pBuffer;
        raddr2 = convertVirtualtoPhysicalAddress(vaBuffer, &indexPage, &offsetBlock);

        //
        if (!raddr2)
        {
            int __errno = swap(getLeastActivePageInMemory(), &pageTable[indexPage]);
            raddr2 = convertVirtualtoPhysicalAddress(ptr, &indexPage, &offsetBlock);
        }
        //

        for (j = 0; j < szBuffer; j++)
            raddr2[j] = raddr[j];
        //vaBuffer[j] = raddr[j];
        pBuffer = (void*) vaBuffer;
        return SUCCESSFUL_CODE;
    }

    else
    {
        return OUT_OF_RANGE_ERROR;
    }
}

int _write (VA ptr, void* pBuffer, size_t szBuffer)
{
    VA raddr, vaBuffer;
    int indexPage, offsetBlock;
    struct block* readBlock;
    unsigned int j;
    if (!ptr || !pBuffer || (szBuffer > (unsigned int) sizePage))
    {
        return INCORRECT_PARAMETERS_ERROR;
    }
    raddr = convertVirtualtoPhysicalAddress(ptr, &indexPage, &offsetBlock);
    if (!raddr)
    {
        int __errno = swap(getLeastActivePageInMemory(), &pageTable[indexPage]);
        raddr = convertVirtualtoPhysicalAddress(ptr, &indexPage, &offsetBlock);
    }
    readBlock = findBlock(indexPage, offsetBlock);
    if (szBuffer <= readBlock -> szBlock)
    {
        vaBuffer = (VA) pBuffer;
        for (j = 0; j < szBuffer; j++)
            raddr[j] = vaBuffer[j];
        pageTable[indexPage].activityFactor++;
        //___print_memory();
        return SUCCESSFUL_CODE;
    }

    else
    {
        return OUT_OF_RANGE_ERROR;
    }

}

int _init (int n, int szPage)
{
    remove(SWAPING_NAME);
    if (n > 0 && szPage > 0)
    {
        sizePage = szPage;
        numberOfPages = n;
        pageTable = (page*) malloc(sizeof(page) * n);
        allocbuf = (VA) malloc(sizeof(char) * ALLOCSIZE);
        nullMemory();
        if (allocbuf && pageTable)
        {
            return initPageTable();
        }
        else
        {
            return UNKNOWN_ERROR;
        }
    }
    return INCORRECT_PARAMETERS_ERROR;
}

/**
 	@func	initPageTable
 	@brief	Èíèöèàëèçàöèÿ òàáëèöû ñòðàíèö

	@return	êîä îøèáêè
	@retval	0	óñïåøíîå âûïîëíåíèå
	@retval	1	íåèçâåñòíàÿ îøèáêà
 **/
int initPageTable()
{
    int numberOfSwapPages = 0;
    int numberOfRAMPages = 0;
    int __errno;
    int i;

    for (i = 0; i < numberOfPages; i++)
    {
        // èçíà÷àëüíî ó êàæäîé ñòðàíèöû åñòü åäèíñòâåííûé ñâîáîäíûé áëîê ñ ðàçìåðîì, ðàâíûì ðàçìåðó ñòðàíèöû
        // âûäåëÿåì ïàìÿòü ïîä ñâîáîäíûé áëîê è èíèöèàëèçèðóåì åãî
        struct block* firstBlock = (struct block *) malloc(sizeof(struct block));
        firstBlock -> pNext = NULL;
        firstBlock -> szBlock = sizePage;
        firstBlock -> offsetBlock = 0;

        // èíèöèàëèçèðóåì ñòðàíèöó
        pageTable[i].pFirstFree = firstBlock;
        pageTable[i].pFirstUse = NULL;
        pageTable[i].maxSizeFreeBlock = sizePage;
        pageTable[i].activityFactor = 0;

        // åñëè ïàìÿòè õâàòàåò, òî ïèøåì ñòðàíèöó â ïàìÿòü, èíà÷å íà äèñê
        if ((i+1) * sizePage <= ALLOCSIZE)
        {
            pageTable[i].pInfo.offsetPage = numberOfRAMPages++;
            pageTable[i].pInfo.isUse = TRUE;
            // óêàçàòåëü íà íà÷àëî ïåðâîãî áëîêà ïðè èíèöèàëèçàöèè ðàâåí ôèçè÷åñêîìó àäðåñó ñòðàíèöû
            //pageTable[i].pFirstFree -> allocp = convertVirtualtoPhysicalAddress(pageTable[i].pInfo.offsetPage);
        }
        else
        {
            __errno = writef(numberOfSwapPages, allocbuf);	// ??? íàäî ëè çàïîìèíàòü èíèöèàëèçèðîâàííóþ ïàìÿòü?????
            if (!__errno)
            {
                pageTable[i].pInfo.offsetPage = numberOfSwapPages++;
                pageTable[i].pInfo.isUse = FALSE;
                // ò.ê. óæå âûøëè çà ïðåäåëû ôèçè÷åñêîé ïàìÿòè, òî óñòàíàâëèâàåì óêàçàòåëü íà íà÷àëî ïàìÿòè
                // ïðè ñòðàíè÷íîì ïðåðûâàíèè îí áóäåò ïåðåñ÷èòûâàòüñÿ â çàâèñèìîñòè îò íîâîãî ñìåùåíèÿ ñòðàíèöû
                //pageTable[i].pFirstFree -> allocp = allocbuf;
            }
            else
            {
                return UNKNOWN_ERROR;
            }
        }
    }
    return SUCCESSFUL_CODE;
}

/**
 	@func	___print_memory
 	@brief	Âûâåñòè ñîäåðæèìîå ôèçè÷åñêîé ïàìÿòè íà êîíñîëü (äëÿ òåñòèðîâàíèÿ)
 **/
void ___print_memory()
{
    int i;
    for (i = 0; i < ALLOCSIZE; i++)
    {
        printf("%c", allocbuf[i]);
    }
    printf("%c", '\n');
}

/**
 	@func	addToUsedBlocks
 	@brief	Äîáàâëÿåò áëîê â ñïèñîê çàíÿòûõ áëîêîâ óêàçàííîé ñòðàíèöû
	@param	[in] _block			óêàçàòåëü íà áëîê
	@param	[in] _page			óêàçàòåëü íà ñòðàíèöó
 **/
void addToUsedBlocks(struct block *_block, struct page *_page)
{
    struct block *usedBlockPtr = _page -> pFirstUse;
    struct block *parentBlockPtr = NULL;
    while (usedBlockPtr)
    {
        parentBlockPtr = usedBlockPtr;
        usedBlockPtr = usedBlockPtr -> pNext;
    }
    if (parentBlockPtr)
    {
        parentBlockPtr -> pNext = _block;
    }
    else
    {
        _page -> pFirstUse = _block;
    }
}

void addToFreeBlocks(struct block *_block, struct page *_page)
{
    struct block *freeBlockPtr = _page -> pFirstFree;
    struct block *parentBlockPtr = NULL;
    // íàõîäèì êðàéíèé ñâîáîäíûé áëîê
    while (freeBlockPtr)
    {
        parentBlockPtr = freeBlockPtr;
        freeBlockPtr = freeBlockPtr -> pNext;
    }
    // åñëè íàøëè, òî äîáàâëÿåì îñâîáîæä¸ííûé áëîê â êîíåö ñâîáîäíûõ áëîêîâ
    if (parentBlockPtr)
    {
        // åñëè ñâîáîäíûå áëîêè èäóò äðóã çà äðóãîì, òî èõ ñêëåèâàåì
        if ((parentBlockPtr -> offsetBlock + parentBlockPtr -> szBlock)
            == _block -> offsetBlock)
        {
            parentBlockPtr -> szBlock += _block -> szBlock;
            free(freeBlockPtr);
        }
        else
        {
            parentBlockPtr -> pNext = _block;
            _block -> pNext = NULL;
        }
    }
    else
    {
        _page -> pFirstFree = _block;
    }
}

/**
 	@func	mallocInPage
 	@brief	Âûäåëÿåò áëîê ïàìÿòè îïðåäåëåííîãî ðàçìåðà â îïðåäåë¸ííîé ñòðàíèöå
	@param	[out] ptr		àäðåññ áëîêà
	@param	[in]  szBlock	ðàçìåð áëîêà
	@param	[in]  _page		àäðåñ ñòðàíèöû

	@return	êîä îøèáêè
	@retval	0	óñïåøíîå âûïîëíåíèå
	@retval	1	íåèçâåñòíàÿ îøèáêà
 **/
int mallocInPage(VA* ptr, size_t szBlock, int pageNum)
{
    struct page *_page = &pageTable[pageNum];
    struct block *blockPtr = _page -> pFirstFree;
    struct block *parentBlockPtr = NULL;
    // èùåì ïîäõîäÿùèé áëîê
    while (blockPtr)
    {
        // åñëè íàøëè ïîäõîäÿùèé ïî ðàçìåðó áëîê
        if (blockPtr -> szBlock >= szBlock)
        {
            int vaddr = pageNum * sizePage + blockPtr -> offsetBlock + 1;
            *ptr = (VA) (vaddr);	// ïðèáàâëÿåì åäèíèöó, ÷òîáû óêàçàòåëü íà ïåðâûé áëîê íå áûë íóëåâûì
            if (blockPtr -> szBlock == szBlock)
            {
                // ñâîáîäíûé áëîê çàíÿëè ïîëíîñòüþ
                if (parentBlockPtr)
                    parentBlockPtr -> pNext = blockPtr -> pNext;
                else
                    _page -> pFirstFree = NULL;
                blockPtr -> pNext = NULL;
                addToUsedBlocks(blockPtr, _page);
            }
            else
            {
                // ñîçäà¸ì íîâûé çàíÿòûé áëîê
                struct block* usedBlock;
                usedBlock = (struct block *) malloc(sizeof(struct block));
                usedBlock -> offsetBlock = blockPtr -> offsetBlock;
                usedBlock -> pNext = NULL;
                usedBlock -> szBlock = szBlock;
                addToUsedBlocks(usedBlock, _page);

                // ñâîáîäíûé áëîê ñóæàåòñÿ
                blockPtr -> offsetBlock += szBlock;
                blockPtr -> szBlock -= szBlock;
            }
            // îáíîâëÿåì èíôîðìàöèþ î ñòðàíèöå
            _page -> maxSizeFreeBlock = getMaxSizeFreeBlock(*_page);
            _page -> activityFactor++;
            return SUCCESSFUL_CODE;
        }
        else
        {
            // áëîê åù¸ íå íàéäåí
            parentBlockPtr = blockPtr;
            blockPtr = blockPtr -> pNext;
        }
    }
    return UNKNOWN_ERROR;
}

/**
 	@func	nullMemory
 	@brief	Îáíóëÿåò ïàìÿòü
 **/
void nullMemory()
{
    //memset(allocbuf, NULL_SYMBOL, ALLOCSIZE);
    int i;
    for (i = 0; i < ALLOCSIZE; i++)
    {
        allocbuf[i] = NULL_SYMBOL;
    }
}

/**
 	@func	getMaxSizeFreeBlock
 	@brief	Íàõîäèò ìàêñèìàëüíóþ äëèíó ñâîáîäíîãî áëîêà â ñòðàíèöå
	@param	[in] _page			_ñòðàíèöà
	@return	ìàêñèìàëüíàÿ äëèíó ñâîáîäíîãî áëîêà â ñòðàíèöå
 **/
unsigned int getMaxSizeFreeBlock(struct page _page)
{
    unsigned int maxSize = 0;
    struct block *blockPtr = _page.pFirstFree;
    while (blockPtr)
    {
        if (blockPtr -> szBlock > maxSize)
        {
            maxSize = blockPtr -> szBlock;
        }
        blockPtr = blockPtr -> pNext;
    }
    return maxSize;
}

/**
 	@func	getLeastActivePageInMemory
 	@brief	Íàõîäèò íàèìåíåå àêòèâíóþ ñòðàíèöó â ïàìÿòè
	@return	íàèìåíåå àêòèâíàÿ ñòðàíèöà
 **/
struct page* getLeastActivePageInMemory()
{
    int i;
    int minActivityIndex;
    struct page* minActivityPage = NULL;
    for (i = 0; i < numberOfPages; i++)
    {
        // åñëè ñòðàíèöà â ïàìÿòè, è å¸ êîýôôèöèåíò àêòèâíîñòè îêàçàëñÿ ìåíüøå
        if (pageTable[i].pInfo.isUse)
        {
            if (!minActivityPage ||
                pageTable[i].activityFactor < minActivityPage -> activityFactor)
            {
                minActivityPage = &pageTable[i];
                minActivityIndex = i;
            }
        }
    }
    return minActivityPage;
}

/**
 	@func	convertVirtualtoPhysicalAddress
 	@brief	Êîíöåðòàöèÿ âèðòóàëüíîãî àäðåñà â ôèçè÷åñêèé
	@param	[in] ptr			âèðòóàëüíûé àäðåñ
	@param	[out] offsetPage	ñìåùåíèå ñòðàíèöû
	@param	[out] offsetBlock	ñìåùåíèå áëîêà
	@return	ôèçè÷åñêèé àäðåñ
 **/
VA convertVirtualtoPhysicalAddress(VA ptr, int *offsetPage, int *offsetBlock)
{
    struct block *trueBlock, *tempBlock;
    int maxBlockOffset;
    const int SIGN_DIFFERENCE = 256;
    int vaddr = (int)ptr - 1;		// îòíèìàåì åäèíèöó, êîòîðóþ ïðèáàâèëè ïðè âûäåëåíèè áëîêà (÷òîáû óêàçàòåëü íà ïåðâûé áëîê íå áûë íóëåâûì)
    VA allocp = NULL;
    //int offset;
    if (vaddr < 0)
    {
        vaddr = SIGN_DIFFERENCE * sizeof(char) + vaddr;
    }
    //offset = (int) (log((double)sizePage) / log(2.0));
    //offsetPage = vaddr >> offset;
    *offsetPage = vaddr / sizePage;
    *offsetBlock = vaddr % sizePage;

    if (*offsetPage <= numberOfPages && pageTable[*offsetPage].pInfo.isUse)
    {
        allocp = allocbuf + pageTable[*offsetPage].pInfo.offsetPage * sizePage + *offsetBlock;
    }

    tempBlock = pageTable[*offsetPage].pFirstUse;
    maxBlockOffset = tempBlock -> offsetBlock;
    while (tempBlock && tempBlock -> pNext && tempBlock -> pNext -> offsetBlock <= *offsetBlock)
        tempBlock = tempBlock -> pNext;
    *offsetBlock = tempBlock -> offsetBlock;
    return allocp;
}

/**
 	@func	findBlock
 	@brief	Íàõîäèò áëîê ñ óêàçàííûì ñìåùåíèåì â óêàçàííîé ñòðàíèöå
	@param	[in] offsetPage	ñìåùåíèå ñòðàíèöû
	@param	[in] offsetBlock	ñìåùåíèå áëîêà
	@return	óêàçàòåëü íà èñêîìûé áëîê
 **/
struct block* findBlock(int offsetPage, int offsetBlock)
{
    struct block* blockPtr = pageTable[offsetPage].pFirstUse;
    while (blockPtr && blockPtr -> offsetBlock != offsetBlock)
    {
        blockPtr = blockPtr -> pNext;
    }
    return blockPtr;
}

struct block* findParentBlock(int offsetPage, int offsetBlock)
{
    struct block* parentBlock = NULL;
    struct block* blockPtr = pageTable[offsetPage].pFirstUse;
    while (blockPtr && blockPtr -> offsetBlock != offsetBlock)
    {
        parentBlock = blockPtr;
        blockPtr = blockPtr -> pNext;
    }
    return parentBlock;
}

/**
 	@func	swap
 	@brief	Ïðîöåäóðà ñòðàíè÷íîãî ïðåðûâàíèÿ
	@param	[in] memoryPage		ñòðàíèöà â ïàìÿòè
	@param	[in] swopPage		ñòðàíèöà íà äèñêå
	@return	êîä îøèáêè
	@retval	0	óñïåøíîå âûïîëíåíèå
	@retval	-1	ñòðàíèöà íà äèñêå íå íàéäåíà
	@retval	-2	íåîæèäàííûé êîíåö ôàéëà
 **/
int swap(struct page *memoryPage, struct page *swopPage)
{
    const int add_offset = 2;
    struct pageInfo bufInfo;
    int __errno, j;

    VA memoryPtr = allocbuf + memoryPage -> pInfo.offsetPage * sizePage - add_offset;
    VA memoryPageContent = (VA) malloc(sizeof(char) * sizePage);

    for (j = 0; j < sizePage; j++)
        memoryPageContent[j] = memoryPtr[j];
    __errno = readf(swopPage -> pInfo.offsetPage, memoryPtr);
    if (!__errno)
        __errno = writef(swopPage -> pInfo.offsetPage, memoryPageContent);

    bufInfo = memoryPage -> pInfo;
    memoryPage -> pInfo = swopPage -> pInfo;
    swopPage -> pInfo = bufInfo;
    memoryPage -> activityFactor++;
    swopPage -> activityFactor++;
    return __errno;
}

/**
 	@func	readf
 	@brief	×èòàåò ñòðàíèöó èç ôàéëà (ñâîïèíãà)

	@param	[in]  offsetPage	ñìåùåíèå ñòðàíèöû â ôàéëå
	@param	[out] readPage		ìåñòî äëÿ ïðî÷èòàííîé ñòðàíèöû

	@return	êîä îøèáêè
	@retval	0	óñïåøíîå âûïîëíåíèå
	@retval	-1	ñòðàíèöà íå íàéäåíà
	@retval	-2	íåîæèäàííûé êîíåö ôàéëà
 **/
int readf(unsigned long offsetPage, VA readPage)
{
    const int FILE_NOT_FOUND = -1;
    const int UNEXPECTED_EOF = -2;

    FILE *fp;
    int returnValue;
    long fileOffset;
    int __errno;

    fileOffset = offsetPage * sizePage;
    __errno = fopen_s(&fp, SWAPING_NAME, "r");
    if (!__errno)
    {
        __errno = fseek(fp, fileOffset, SEEK_SET);
        if (!__errno/* && fgets(readPage, sizePage + 1, fp)*/)
        {

            int i;
            for (i = 0; i < sizePage; i++)
                readPage[i] = fgetc(fp);
            returnValue = SUCCESSFUL_CODE;
        }
        else
        {
            returnValue = UNEXPECTED_EOF;
        }
        fclose(fp);
    }
    else
    {
        returnValue = FILE_NOT_FOUND;
    }
    return returnValue;
}

/**
 	@func	writef
 	@brief	Ïèøåò ñòðàíèöó â ôàéë (ñâîïèíã)

	@param	[in] offsetPage		ñìåùåíèå ñòðàíèöû â ôàéëå
	@param	[in] writtenPage	ñîäåðæèìîå ñòðàíèöû

	@return	êîä îøèáêè
	@retval	0	óñïåøíîå âûïîëíåíèå
	@retval	-1	ñòðàíèöà íå íàéäåíà
	@retval	-2	íåîæèäàííûé êîíåö ôàéëà
 **/
int writef(unsigned long offsetPage, VA writtenPage)
{
    const int FILE_NOT_FOUND = -1;
    const int UNEXPECTED_EOF = -2;

    FILE *fp;
    int returnValue;
    long fileOffset;
    int __errno;

    fileOffset = offsetPage * sizePage;
    __errno = fopen_s(&fp, SWAPING_NAME, "r+");		// îòêðûâàåì äëÿ ìîäèôèêàöèè
    if (__errno == 2)
    {
        // åñëè ïîïàëè ñþäà, çíà÷èò ñâîï åù¸ íå ñîçäàí, ñîçäà¸ì åãî
        __errno = fopen_s(&fp, SWAPING_NAME, "w");
    }
    if (!__errno)
    {
        __errno = fseek(fp, fileOffset, SEEK_SET);
        if (!__errno /*&& fputs(writtenPage, fp) >= 0*/)
        {
            int i;
            for (i = 0; i < sizePage; i++)
                fputc(writtenPage[i], fp);
            returnValue = SUCCESSFUL_CODE;
        }
        else
        {
            returnValue = UNEXPECTED_EOF;
        }
        fclose(fp);
    }
    else
    {
        returnValue = FILE_NOT_FOUND;
    }
    return returnValue;
}

void _printBuffer(VA ptr, size_t szBuffer)
{
    VA raddr, vaBuffer;
    int indexPage, offsetBlock;
    unsigned int j;
    struct block* readBlock;
    raddr = convertVirtualtoPhysicalAddress(ptr, &indexPage, &offsetBlock);
    // åñëè íå ïîëó÷èëè àäðåñ, òî íóæíûé áëîê â ñâîïå
    // âûïîëíÿåì ñòðàíè÷íîå ïðåðûâàíèå
    if (!raddr)
    {
        int __errno = swap(getLeastActivePageInMemory(), &pageTable[indexPage]);
        raddr = convertVirtualtoPhysicalAddress(ptr, &indexPage, &offsetBlock);
    }
    //___print_memory();
    //readBlock = findBlock(indexPage, offsetBlock);
    //if (szBuffer <= readBlock -> szBlock)
    {
        //vaBuffer = (VA) pBuffer;
        for (j = 0; j < szBuffer; j++)
            printf("%c", raddr[j]);
        //vaBuffer[j] = raddr[j];
        //pBuffer = (void*) vaBuffer;
        //return SUCCESSFUL_CODE;
    }
}