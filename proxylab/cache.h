/*
 * cache.h
 * 周钊平
 * guest377
 * 1100012779
 */
#include "csapp.h"

#define CACHED 1
#define UNCACHED 2
#define NOCACHE 3
#define CACHE_SUCCESS 4
#define CACHE_FAILURE 5
#define CACHE_BY_OTHER 6
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* cacheu元素的结构 */
struct cdata_t
{
        char *url;
        int size;
        unsigned long logic_time; 
        void *ptr;  //指向缓存
        int readcnt;
        sem_t cmutex; /* mutex for read count */
        struct cdata_t *next;//上一个cache
        struct cdata_t *prev;//下一个cache
};

typedef struct cdata_t cdata;

/*
 * 查询是否存在url缓存，如果存在则将缓存返回浏览器
 * 并且返回值 CACHED
 * 否则则返回 UNCACHED
 */
int Get_cache(char *url, int browserfd);
/*
 * 向链表中添加一个cache元素
 */
int Create_cache(char *url, char *ptr, int size);
/*
 * cache链表的初始化
 */
void Cache_init();
