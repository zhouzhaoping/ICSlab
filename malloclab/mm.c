/*
 * mm.c
 *
 * <guest377>
 * 周钊平
 * mail:1100012779@pku.edu.cn
 *
 * overviw:（程序中的注释中英文混杂了，请见谅）
 *
 * 0、我使用的是分离链表、首次适配、直接合并
 * 
 * 1、对于每个最小块(16字节）我分为(每个中括号内是四个字节）：
 *   已分配块：[head][content][content][foot]   空闲块：[head][pred][succ][foot]
 *           头信息       内容        脚信息       头信息|前驱空闲|后继空闲|脚信息
 * 其中pred、succ两个地方我没有保存前驱和后继的指针，因为每次实验所开堆不会大于等于2^32，
 * 所以在pred、succ两个地方只需要保存此块与前后两个空闲块的偏移位置（4字节）就行了，
 * 这样子最小块就只有16字节，节省了空闲。
 * 
 * 2、对于分离链表我用二的幂次将其分为20类【[16,32)、[32,64)、[2^4,2^5) …、[2^n,2^(n+1))…、[2^23,infinite)】
 * 找寻适配空闲块时由于有二的幂次的存在，可以用位运算加速。 
 *
 * 3、每个链表都由两个值管理（链表头free_list_p[i], 链表尾free_list_t[i]，分别指向链表的头和尾)。
 * 链表头数组为free_list_p,链表尾数组为free_list_t,初始化时在堆的最开始处开了40 * DSIZE保存这两个指针。
 * 如果链表为空那么free_list_p = free_list_t = NULL。
 * 
 * 4、链表的存储顺序按照地址顺序。
 * 链表的主要操作为insert和remove，分别向分离链表中插入/删除一个空闲块
 * 
 * 5、合并的方式为直接合并
 * 如果产生一个空闲块，那么就检查他的前后块，如果有空闲的那么就直接合并
 * 合并分成四种情况，以前后块是否为空闲块来分类
 * 
 * 6、适配方式为首次适配（搜索每个分离链表时，只要遇到合适的就直接适配）
 *
 * 7、在请求更大空间的extend_heap中，如果堆的最后一块为空闲，那么可以适当减少需要扩张的size值
 * 
 * 8、用宏和inline封装比较常用的函数、计算，提高封装性，方便计算
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
//#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */


/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Doubleword size (bytes) */
#define TSIZE       12
#define QSIZE       16
#define MINBLOCK    16
#define INFOSIZE    8
#define CHUNKSIZE   (1 << 8)  /* Extend heap by this amount (bytes) */ 

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) 

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            
#define PUT(p, val)  (*(unsigned int *)(p) = (val))  

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   
#define GET_ALLOC(p) (GET(p) & 0x1)                  

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Given block ptr bp, compute size/pred/succ */
#define SIZE(bp)      (GET_SIZE(HDRP(bp)))
#define PREV_SIZE(bp) (GET_SIZE((char *)(bp) - DSIZE))
#define NEXT_SIZE(bp) (GET_SIZE((char *)(bp) + SIZE(bp) - WSIZE))
#define ALLOC(bp)     (GET_ALLOC(HDRP(bp)))
#define PREV_ALLOC(bp) (GET_ALLOC((char *)(bp) - DSIZE))
#define NEXT_ALLOC(bp) (GET_ALLOC((char *)(bp) + SIZE(bp) - WSIZE))
#define PRED(bp)      ((char *)(bp) - GET(bp))
#define SUCC(bp)      ((char *)(bp) + GET((char *)(bp) + WSIZE))

/* Given block ptr bp, change pred/succ */
#define PUT_PRED(bp, pre)  PUT(bp, (unsigned int)((char *)(bp) - (char *)(pre)))
#define PUT_SUCC(bp, suc)  PUT((char *)(bp) + WSIZE, (unsigned int)((char *)(suc) - (char *)(bp)))


/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */
static size_t *free_list_p;  /* 链表头数组,第i个链表头为free_list_p[i] */
static size_t *free_list_t;  /* 链表尾数组,第i个链表尾为free_list_t[i] */

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t asize);    //请求更大的空间
static void *place(void *bp, size_t asize);//拆分块
static void *find_fit(size_t asize);       //寻找适配的空闲块
static void *coalesce(void *bp);           //合并空闲块
static void printblock(void *bp);          //输出块信息
static void checkblock(void *bp);          //检查块
static void *insertblock(void *bp);        //插入一个新的空闲块到空闲链表中 
static void removeblock(void *bp);         //删除空闲链表中的某个空闲块
static void checkfreelist(void);           //检查每个空闲链表
static int blockindex(unsigned int size);  //计算块所在的链表的索引

/*
 * mm_init - Called when a new trace starts.
 * 堆和一些变量的初始化
 */
int mm_init(void) 
{
	/* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(40 * DSIZE + QSIZE)) == (void *)-1) //line:vm:mm:begininit
		return -1;
    
	/* 分离链表初始化 */
	free_list_p = (size_t *)heap_listp;
	free_list_t = (size_t *)(heap_listp + 20 * DSIZE);
	int i;
	for (i = 0; i < 20; ++i){
		free_list_p[i] = (size_t)NULL;
		free_list_t[i] = (size_t)NULL;
	}
	heap_listp += 40 * DSIZE;   

	/* heap的头尾封装 */
	PUT(heap_listp, 0);                      /* Alignment padding */
    PUT(heap_listp + WSIZE, PACK(DSIZE, 1)); /* Prologue header */ 
    PUT(heap_listp + DSIZE, PACK(DSIZE, 1)); /* Prologue footer */ 
    PUT(heap_listp + TSIZE, PACK(0, 1));     /* Epilogue header */  
	heap_listp += DSIZE;
	

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE) == NULL) 
		return -1;
	return 0;
}

/*
 * malloc - Allocate a block.
 * 先从空闲链表中找到最佳适配的空闲块，如果没有就请求更大的空间
 *
 */
void *mm_malloc(size_t size) 
{
	size_t asize;      /* Adjusted block size */
    char *bp;      
    
	/* Adjust block size to include overhead and alignment reqs. */
    if (size + INFOSIZE <= MINBLOCK)                    
		asize = MINBLOCK;                                     
    else
		asize = DSIZE * ((size + INFOSIZE + (DSIZE - 1)) / DSIZE); 
	
	/* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
		bp = place(bp, asize);
		return bp;
    }

    /* No fit found. Get more memory and place the block */
	if (asize > CHUNKSIZE){
		/* big size - require asize directly */ 
		if ((bp = extend_heap(asize)) == NULL)  
			return NULL;
		/* simplified place */
		removeblock(bp);
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		return bp;
	}
	else{
		/* require CHUNKSIZE */
		if ((bp = extend_heap(CHUNKSIZE)) == NULL)  
				return NULL;
		place(bp, asize);
		return bp;
	}
}

/*
 * free - free a allocted block.
 * 将已分配的块重新包装成空闲块，并且插入到空闲链表里
 */
void mm_free(void *bp)
{
	if(bp == NULL) 
		return;

    size_t size = SIZE(bp);

	/* repack the block into freestate */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

	/* insert bp into free_list and coalesce*/
	coalesce(insertblock(bp));
}


/*
 * realloc - Change the size of the block by mallocing a new block,
 *      copying its data, and freeing the old block.
 * 1、如果ptr为NULL,相当于malloc
 * 2、如果size为0，相当于free
 * 3、如果原来的块已经足够，那么考虑剩余的部分是否能拆分成空闲块
 * 4、如果后面的块和原来的块的总和已经足够，那么考虑剩余的部分是否能拆分成空闲块
 * 5、malloc一个块，然后将数据复制过去
 *
 */
void *mm_realloc(void *ptr, size_t size)
{
	size_t oldsize, asize, freesize;
	char *free_bp, *next;
    void *newptr;

	/* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL)
		return mm_malloc(size);
    
	/* If size == 0 then this is just free, and we return NULL. */
    if(size == 0){
		mm_free(ptr);
		return NULL;
	}

    oldsize = SIZE(ptr);
	if (size + INFOSIZE <= MINBLOCK)                    
		asize = MINBLOCK;                                     
    else
		asize = DSIZE * ((size + INFOSIZE + (DSIZE - 1)) / DSIZE);

    if(oldsize >= asize){
		/* oldsize is enough */
		if (oldsize - asize >= MINBLOCK){
			/* turn remainder size into free block */
			if (!NEXT_ALLOC(ptr)){
				/* if next is free, can coalesce */
				oldsize += NEXT_SIZE(ptr);
				removeblock(NEXT_BLKP(ptr));
		    }
			/* remainder is free */
			PUT(HDRP(ptr), PACK(asize, 1));
			PUT(FTRP(ptr), PACK(asize, 1));
			/* repack free-block */
			free_bp = NEXT_BLKP(ptr);
			PUT(HDRP(free_bp), PACK(oldsize - asize, 0));
			PUT(FTRP(free_bp), PACK(oldsize - asize, 0));
			/* relink free-block */
			insertblock(free_bp);
		}
		return ptr;
	}
	else if (!NEXT_ALLOC(ptr) && NEXT_SIZE(ptr) + oldsize >= asize){
		/* next's a free-block and can fill the gap */
		next = NEXT_BLKP(ptr);
		removeblock(next);
		if (SIZE(next) + oldsize - asize >= MINBLOCK){
			/* remainder is big than MINBLOKP, can turn into a free-block */
			freesize = SIZE(next) + oldsize - asize;
			/* repack the alloc-block */
			PUT(HDRP(ptr), PACK(asize, 1));
			PUT(FTRP(ptr), PACK(asize, 1));
			/* repack the free-block */
			free_bp = NEXT_BLKP(ptr);
			PUT(HDRP(free_bp), PACK(freesize, 0));
			PUT(FTRP(free_bp), PACK(freesize, 0));
			/* relink the free-block */
			insertblock(free_bp);
		}
		else{
			/* the all next-block and oldblock turn to a alloc-block */
			PUT(HDRP(ptr), PACK(SIZE(next) + oldsize, 1));
			PUT(FTRP(ptr), PACK(SIZE(next) + oldsize, 1));
		}
		return ptr;
	}
	else{
		/* just malloc one */
		newptr = mm_malloc(size);
		memcpy(newptr, ptr, size);//move block's data
		mm_free(ptr);
		return newptr;
	}
}

/* 
 * The remaining routines are internal helper routines 
 */

/*
 * calloc - Allocate the block and set it to zero.
 * 分配一个size块，并且初始化为零
 */
void *calloc (size_t nmemb, size_t size)
{
   size_t bytes = nmemb * size;
   void *newptr;
   /* malloc */
   newptr = mm_malloc(bytes);
   /* initial */
   memset(newptr, 0, bytes);

   return newptr;
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 * 合并的前提：bp为一个空闲块，在链表里
 * 合并分成四种情况1、前后都是已分配块，不需要合并
 *                 2、后面是空闲块，删除后面的块，与bp合并成一个大的块再插入
 *                 3、前面是空闲块，删除前面的块，与bp合并成一个大的块插入
 *                 4、前后都是空闲块，删除两块，与bp合并成一个大的块插入
 */
static void *coalesce(void *bp) 
{	
	size_t prev_alloc = PREV_ALLOC(bp);
    size_t next_alloc = NEXT_ALLOC(bp);
    size_t size = SIZE(bp);

    if (prev_alloc && next_alloc)              /* Case 1 */
		return bp;

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
		char *next = NEXT_BLKP(bp);
		size += SIZE(next);
		/* delete link */
		removeblock(next);
		removeblock(bp);
		/* repack bp */
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));
		/* relink */
		insertblock(bp);
		return bp;
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
		char *prev = PREV_BLKP(bp);
		size += SIZE(prev);
		/* delete link */
		removeblock(prev);
		removeblock(bp);
		/* repack bp */
		PUT(HDRP(prev), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
		/* relink */
		insertblock(prev);
		return prev;
    }

    else {                                     /* Case 4 */
		char *prev = PREV_BLKP(bp);
		char *next = NEXT_BLKP(bp);
		size += SIZE(prev) + SIZE(next);
		/* delete link */
		removeblock(prev);
		removeblock(next);
		removeblock(bp);
		/* repack bp */ 
		PUT(HDRP(prev), PACK(size, 0));
		PUT(FTRP(next), PACK(size, 0));
		/* relink */
		insertblock(prev);
		return prev;
    }
}

/* 
 * insertblock - insert bp into free-list 
 * 插入的前提：bp是一个空闲块，但是不在空闲链表里
 * 链表的管理方式是按地址顺序
 */
static inline void *insertblock(void *bp){
	
	int i = blockindex(SIZE(bp));

	if (free_list_p[i] == (size_t)NULL)
		/* the free one is the only free-block */
		free_list_p[i] = free_list_t[i] = (size_t)bp;
	else{
		if ((size_t)(bp) < free_list_p[i]){
			/* bp has the min-adrr, trun to free_list_p */
			PUT_SUCC(bp, free_list_p[i]);
			PUT_PRED(free_list_p[i], bp);
			free_list_p[i] = (size_t)bp;
		}
		else if ((size_t)(bp) > free_list_t[i]){
			PUT_PRED(bp, free_list_t[i]);
			PUT_SUCC(free_list_t[i], bp);
			free_list_t[i] = (size_t)bp;
		}
		else{
			char *temp = (char *)free_list_p[i];
			/* find pred of bp */
			while (SUCC(temp) < (char *)bp)
				temp = SUCC(temp);
			/* relink bp */
			PUT_PRED(bp, temp);
			PUT_SUCC(bp, SUCC(temp));
			PUT_PRED(SUCC(temp), bp);
			PUT_SUCC(temp, bp);
		}
	}
	return bp;
}

/* 
 * removeblock - remove bp out of free-list 
 * 删除链表结点的方式：
 * 1、如果结点是链表的头/尾，直接重新标记链表的头尾
 * 2、如果结点不是头尾，将头尾的前驱和后继连接在一块
 */
static inline void removeblock(void *bp){
	
	int i = blockindex(SIZE(bp));

	if (free_list_p[i] == free_list_t[i]){
		/* the free one is the only free-block */
		free_list_p[i] = free_list_t[i] = (size_t)NULL;
	}
	else if ((size_t)(bp) == free_list_p[i])
		/* bp is free_list's head */
		free_list_p[i] = (size_t)SUCC(bp);
	else if ((size_t)(bp) == free_list_t[i])
		/* bp is free_list's tail*/
		free_list_t[i] = (size_t)PRED(bp);
	else{
		/* else relink bp's pred/succ */
		PUT_PRED(SUCC(bp), PRED(bp));
		PUT_SUCC(PRED(bp), SUCC(bp));
	}
}

/*
 * blockindex - 通过块的大小，返回块所在的分离链表的下标
 * 共分为20组【[16,32)、[32,64)、[2^4,2^5) …、[2^n,2^(n+1))…、[2^23,infinite)】
 * 找寻过程用位运算加速
 */
static inline int blockindex(unsigned int size){
	int i = 0;
	unsigned int mul = 32;
	while (i < 19 && mul < size){
		++i;
		mul <<= 1;
	}
	return i;
}

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 * 新开辟words字节的块（对齐），如果前面有空闲块则利用，并且减少新开辟块的大小
 */
static void *extend_heap(size_t asize) 
{
	char *bp, *prev = (char *)mem_heap_hi() - 7;/* prev:最后一个有效块的脚部 */

	/* save space by use last-block if last-block is free */
	if (!GET_ALLOC(prev)){
		int realneed = (int)(asize - GET_SIZE(prev));
		prev = prev - GET_SIZE(prev) + DSIZE;//move prev from foot to bp(real)
		removeblock(prev);
		if ((bp = mem_sbrk(realneed)) == (void *)-1)  
			return NULL;
		bp = prev;
	}
	else{
		if ((bp = mem_sbrk(asize)) == (void *)-1)  
			return NULL;
	}	

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(asize, 0));         /* Free block header */  
    PUT(FTRP(bp), PACK(asize, 0));         /* Free block footer */  
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ 

	return insertblock(bp);
}

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 * 分配前半部，后面有剩余则保留，没剩余就删除
 * 其实可以改进成分配后半部，保留前半部，如果前半部大小不在原来的分离链表区间中，
 * 那么就调整
 */
static void *place(void *bp, size_t asize)
{
	size_t csize = SIZE(bp);   

	removeblock(bp);
    
	if ((csize - asize) >= MINBLOCK) { 
		size_t freesize = csize - asize;
		
		/* repack the alloc-block */
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));

		/* repack the free-block */
		char *free_block = NEXT_BLKP(bp);;
		PUT(HDRP(free_block), PACK(freesize, 0));
		PUT(FTRP(free_block), PACK(freesize, 0));

		/* relink the free-block */
		insertblock(free_block);
    }
    else {
		/* repack the block */
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
    }

	return bp;
}

/* 
 * find_fit - 在空闲链表里寻找适配asize大小的空闲块 
 * 首次适配
 */
static void *find_fit(size_t asize)
{
	void *bp;
	int i;

	for (i = blockindex(asize); i <= 19; ++i){
		if (free_list_p[i] == (size_t)NULL)
			continue;
		/* find first fit */
		for (bp = (char *)free_list_p[i];; bp = SUCC(bp)) {
			if (SIZE(bp) >= asize){
				return bp;
			}
			if ((size_t)bp == free_list_t[i])
				break;
		}
	}
	return NULL;
}

/*
 * 检查函数，for debug and show the heap
 */

/*
 * mm_checkheap - 检查堆的所有块，并且调用检查空闲链表函数checkfreelist
 *    
 */
void mm_checkheap(int verbose) {

	verbose = verbose;
	printf("\n[check_heap] :\n");
	printf("heap_listp: %p free_list_p: %p free_list_t: %p\n", heap_listp, free_list_p, free_list_t);
	
	/* 检查每个分链表的头尾指针 */
	printf("     free_list_p                  free_list_t\n");
	int i;
	for (i = 0; i < 20; ++i){
		printf("[%02d] %11p %11p ", i, free_list_p + i, (void *)free_list_p[i]);
		printf("[%02d] %11p %11p\n", i, free_list_t + i, (void *)free_list_t[i]);
	}
	printf("\n");

	/* 检查堆链表的头指针 */
	char *bp = heap_listp;
    if ((SIZE(heap_listp) != DSIZE) || !ALLOC(heap_listp))
		fprintf(stderr, "Error: Bad prologue header\n");
    checkblock(heap_listp);

	/* 检查堆中的所有块(分配或空闲) */
    for (bp = heap_listp; SIZE(bp) > 0; bp = NEXT_BLKP(bp)) {
		if (verbose) 
			printblock(bp);
		checkblock(bp);
    }

	/* 检查堆尾 */
    if (verbose)
		printblock(bp);
	if ((SIZE(bp) != 0) || !(GET_ALLOC(HDRP(bp))))
		fprintf(stderr, "Error: Bad epilogue header\n");

	/* 检查分离空闲链表 */
	checkfreelist();
	printf("check over\n");
}
/*
 * printblock - 打印块的信息
 */
static void printblock(void *bp) 
{
    size_t size, alloc;
	size = SIZE(bp);
    alloc = ALLOC(bp);  

    if (size == 0) {
		/* 堆尾打印结束 */
		printf("%p: EOL\n", bp);
		return;
    }

    printf("(%p:%p) alloc/free: [%c] size: [%lu]\n", bp, (char *)FTRP(bp) - WSIZE, (alloc ? 'a' : 'f'), size);
	printf("                          prev: %p next: %p\n", PREV_BLKP(bp), NEXT_BLKP(bp));
	
	/* 空闲块的信息 */
	if (!alloc)
		printf("                          pred: %p succ: %p\n", PRED(bp), SUCC(bp));
}
/*
 * checkblock - 检查一个块
 */
static void checkblock(void *bp) 
{
    /* 检查指针对齐 */
	if ((size_t)bp % 8)
		fprintf(stderr, "Error: %p is not doubleword aligned\n", bp);
	/* 检查头尾相等 */
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
		fprintf(stderr, "Error: header does not match footer\n");
}
/*
 * checkfreelist - 检查空闲链表中的每个结点
 */
static void checkfreelist(void)
{
	printf("\n[check_list] :\n");
	char *temp;
	int i;
	int count = 0;
	/* 检查每条分离链表 */
	for (i = 0; i < 20; ++i){
		temp = (char *)free_list_p[i];
		if (temp == NULL)
			continue;
		printf("list: %d free_list_p: %p free_list_t: %p\n", i, (char *)free_list_p[i], (char *)free_list_t[i]);
		/* 链表头尾指针错误 */
		if (free_list_p[i] > free_list_t[i] || (void *)free_list_p[i] < mem_heap_lo() || (void *)free_list_t[i] > mem_heap_hi())
			fprintf(stderr, "Error: free_list addr error!\n");
		else{
			while (1){
				/* 打印链表中的每一个块 */
				printblock(temp);
				checkblock(heap_listp);
				++count;
				/* 检查分配 */
				if (ALLOC(temp))
					fprintf(stderr, "Error: alloc one in free list!\n");
				/* 检查前后链接 */
				if (temp != (char *)free_list_t[i] && PRED(SUCC(temp)) != temp)
					fprintf(stderr, "Error: block succ can't link it!\n");
				if (temp == (char *)free_list_t[i])
					break;
				else
					temp = SUCC(temp);
			}
		}
	}
	printf("there are %d free-block\n", count);
}
