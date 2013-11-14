/*
 * cache.c
 * 周钊平
 * guest377 
 * 1100012779
 * 
 * 基于读者写者问题
 * 组织方式：链表
 * 替换策略：替换最近不常用的
 */
#include "cache.h"

/* 
 * cache链表
 */
static cdata *head, *rear;
/*
 * mutex for queue
 */
static sem_t qmutex;

static unsigned long cache_time;
/*
 * 总大小
 */
static int tsize; 
/*
 * cache小函数
 */
static int create_cache(cdata* acache);
static void denode(cdata *p);
static void addtorear(cdata *p);
static cdata *get_cache(char *url);
/* 时间设置和读者数量设置函数 */
static unsigned long settime(cdata *acache);
static void setread(cdata *acache);
static int getread(cdata *acache);
static void seturead(cdata *acache);

/* cache链表的初始化 */
void Cache_init()
{
        head = NULL;
        rear = NULL;
        cache_time = 0;
        tsize = 0;
        Sem_init(&qmutex, 0, 1);
}

/*
 * 寻找url的缓存
 */
int Get_cache(char *url, int browserfd)
{
        cdata *acache;
        if((acache = get_cache(url)) == NULL)
                return UNCACHED;

        //write to browser
        if(rio_writen(browserfd, acache->ptr, acache->size) == -1)
                return -1;
        seturead(acache);
        return CACHED;
}
/*
 * 建立一个缓存块缓存网页
 */
int Create_cache(char *url, char *ptr, int size)
{
   
        cdata *acache = (cdata *)malloc(sizeof(cdata));
        int status;

        if(acache)
	{
                acache->url = strdup(url);
                if(!(acache->url))
		{
                        Free(acache);
                        Free(ptr);
                        return -1;
                }
                acache->ptr = realloc(ptr, size); 
                acache->size = size;
                acache->readcnt = 0;
                Sem_init(&(acache->cmutex), 0, 1);
                while((status = create_cache(acache)) == CACHE_FAILURE)
                        sleep(2);
                if(status == CACHE_BY_OTHER)
		{
                        Free(acache->ptr);
                        Free(acache->url);
                        Free(acache);
                }
        }
	else
	{
                Free(ptr);
                return -1;
        }
        return 0;
}
/*
 * 先检查cache链表里是否有同样的url的cache
 * 如果cache链表的总大小超过上限了，则执行替换策略，替换最久的
 */
static int create_cache(cdata* acache)
{
        P(&qmutex); 
        cdata *p = head, *tmp;
        while(p)
	{
                if(!strcmp(p->url, acache->url))
		{ /* has been cached by other threads */
                        V(&qmutex);
                        return CACHE_BY_OTHER;
                }
                p = p->next;
        }

        if((tsize += acache->size) > MAX_CACHE_SIZE)
	{                
                p = head; 
                while(p && tsize > MAX_CACHE_SIZE)
		{
                        /* remove unreaded node from head */
                        if(!getread(p))
			{
                                denode(p);
                                tsize -= p->size;
                                tmp = p;
                                p = p->next; /* update p*/
                                Free(tmp->url); /* free rsrc*/
                                Free(tmp->ptr);
                                Free(tmp);
                        }
			else
			{
                                V(&qmutex);
                                return CACHE_FAILURE;
                        }
                }
        }
        /* enough to add */
        addtorear(acache);
        V(&qmutex);
        return CACHE_SUCCESS;

}

/*
 * 从cache链表中删除cache块p
 */
static void denode(cdata *p)
{
        if(p->prev)
                p->prev->next = p->next;
        if(p->next)
                p->next->prev = p->prev;
        if(p == head)
                head = p->next;
        if(p == rear)
                rear = p->prev;
}

/*
 * 添加cache块p到链表的尾部
 */
static void addtorear(cdata *p)
{
        if(rear == NULL)
	{
                /* an empty list */
                head = rear = p;
                p->prev = NULL;
                p->next = NULL;
        }
	else
	{
                rear->next = p;
                p->prev = rear;
                p->next = NULL;
                rear = p;
        }
        settime(p);
}

/*
 * 从cache链表里找到url的缓存，增加读者数量，然后将其移动到链表尾部
 */
static cdata *get_cache(char *url)
{
        P(&qmutex);
        cdata *p = head;
        while(p)
	{
                if(!strcmp(p->url, url))
		{
                        setread(p);
                        /* move p to rear */
                        if(rear != p)
			{
                                denode(p);
                                addtorear(p);
                        }                       
                        break;
                }
                p = p->next;
        }

        V(&qmutex);
        return p;

}
/*
 * 更新cache链表和cache的时间
 */
static unsigned long settime(cdata *acache)
{
        if(!acache)
                return cache_time;

        acache->logic_time = cache_time++;
        return cache_time;
}

/*
 * 增加读者数量（原子操作）
 */
static void setread(cdata *acache)
{
        P(&(acache->cmutex));
        (acache->readcnt)++;    
        V(&(acache->cmutex));
}

/*
 * 获得读者数量（原子操作）
 */
static int getread(cdata *acache)
{
        int readcnt;
        P(&(acache->cmutex));
        readcnt = (acache->readcnt);    
        V(&(acache->cmutex));
        return readcnt;
}

/*
 * 减少读者数量（原子操作）
 */
static void seturead(cdata *acache)
{
        P(&(acache->cmutex));
        (acache->readcnt)--;    
        V(&(acache->cmutex));
}
