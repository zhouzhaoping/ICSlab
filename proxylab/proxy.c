/*
 * proxy.c
 * 周钊平
 * guest377
 * 1100012779
 *
 */

#include "cache.h"

/*
 * 字符串：
 */
static const char *user_agent = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_str = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection_str = "Connection: close\r\nProxy-Connection: close\r\n";

/*
 * 执行代理/浏览器和服务器之间的通信函数：
 */
void *thread(void *vargp);
int browser2server(rio_t *browser_rp, int client_fd, char *uri, char *host, unsigned short port);
int server2browser(rio_t *server_rp, int browser_fd, char *url);

/*
 * url解析函数:
 */
static int parse_url(const char *url, char *uri, char *host, unsigned short *portp);

/*
 * buf写入缓存/浏览器函数： 
 */
static int buf2cache_browser(int fd, int *tsizep, char **cache, char **pp, char *buf, int length);

/*
 * 主函数：循环接受浏览器请求 
 */
int main(int argc, char **argv)
{
        int listenfd, *connfdp, port;
        socklen_t clientlen;
        struct sockaddr_in clientaddr;
        pthread_t tid;

        Signal(SIGPIPE, SIG_IGN);//避免被SIGPIPE终止
        /* cache初始化 */
        Cache_init();

        if (argc != 2) 
	{
                fprintf(stderr, "usage: %s <port>\n", argv[0]);
                exit(1);
        }
        port = atoi(argv[1]);
        clientlen = sizeof(clientaddr);

        listenfd = Open_listenfd(port);
        while (1) 
	{
                connfdp = (int *)malloc(sizeof(int));
                *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
                fprintf(stderr, "connect with %d\n", *connfdp);
                Pthread_create(&tid, NULL, thread, connfdp);
        }       
        return 0;
}

/*
 * 对等线程：处理浏览器请求
 * 目前只支持GET方法。。。
 * 如果缓存中有了直接返回，否则调用browser2server和server2browser函数
 */
void *thread(void *vargp) 
{
        Pthread_detach(pthread_self());//分离
        int browser_fd = *(int *)vargp;
        Free(vargp);

        rio_t browser_rio, server_rio;
        char buf[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE],
             uri[MAXLINE], host[MAXLINE];

        unsigned short port;
        int client_fd, cstat;

        Rio_readinitb(&browser_rio, browser_fd);

        /* 接受浏览器请求行 */
        if(rio_readlineb(&browser_rio, buf, MAXLINE) == -1)
        {
                Close(browser_fd);
                pthread_exit(NULL);
        }
        sscanf(buf, "%s %s %s", method, url, version);
        fprintf(stderr, "thread: %lu %s\n", pthread_self(), url);
        
        /* 解析请求 */
        if(parse_url(url, uri, host, &port) == -1)
        {
                Close(browser_fd);
                fprintf(stderr, "thread: %lu %s exit: parse uri error\n", pthread_self(), url);
                pthread_exit(NULL);
        }
        
        /* 连接服务器 */
        client_fd = Open_clientfd(host, port);
        if(client_fd == -1)
        {
                Close(browser_fd);
                fprintf(stderr, "thread: %lu %s exit: open conn error\n", pthread_self(), url);
                pthread_exit(NULL);
        }
        Rio_readinitb(&server_rio, client_fd);

        /* 只支持GET方法 */
        if (strcasecmp(method, "GET"))
        { 
                Close(browser_fd);
                Close(client_fd);
                fprintf(stderr, "thread: %lu %s exit: the proxy only can GET method\n", pthread_self(), url);
                pthread_exit(NULL);
        }

        /* GET方法 */
        if((cstat = Get_cache(url, browser_fd)) == -1)
	{
                Close(browser_fd);
                Close(client_fd);
                fprintf(stderr, "thread: %lu %s exit: cached wrt error\n", pthread_self(), url);
                pthread_exit(NULL);
        }
        
        /* 如果有cache */
        if(cstat == CACHED)
	{
                Close(browser_fd);
                Close(client_fd);
                fprintf(stderr, "thread: %lu %s exit: cached\n", pthread_self(), url);
                pthread_exit(NULL);
        }

        /* 接受浏览器请求，传给服务器 */
        if(browser2server(&browser_rio, client_fd, uri, host, port) == -1)
	{
                Close(browser_fd);
                Close(client_fd);
                fprintf(stderr, "thread: %lu %s exit: parse get error\n", pthread_self(), url);
                pthread_exit(NULL);
        }

        /* 接受服务器请求，传给浏览器 */
        server2browser(&server_rio, browser_fd, url);
        Close(browser_fd);
        Close(client_fd);
        fprintf(stderr, "thread: %lu %s exit\n", pthread_self(), url);
        pthread_exit(NULL);
}

/*
 * 从浏览器读入请求，发送给服务器 
 */
int browser2server(rio_t *browser_rp, int client_fd, char *uri, char *host, unsigned short port)
{
        char buf[MAXLINE];
        int n = 0;

        /* 标记，方便发送必要的请求 */
        int flag[5] = {0, 0, 0, 0, 0};
        
        sprintf(buf, "GET %s HTTP/1.0\r\n", uri);//变成1.0
        /* 发送请求 */
        if(rio_writen(client_fd, buf, strlen(buf)) == -1)
                return -1;
        //fprintf(stderr, "%s", buf);

        /* 接受浏览器请求报头，发送给浏览器 */
        while((n = rio_readlineb(browser_rp, buf, MAXLINE)))
	{
                if(n == -1)
                        return -1;

                if(strcasestr(buf, "Host:"))
			flag[0] = 1;
		else if(strcasestr(buf, "Connection:"))
                {
		        strcpy(buf, connection_str);
			flag[1] = 1;
                }
		else if(strcasestr(buf, "User-Agent:"))
		{
                        strcpy(buf, user_agent);
			flag[2] = 1;                
		}
		else if(strcasestr(buf, "Accept:"))
		{
                        strcpy(buf, accept_str);
			flag[3] = 1;                
		}
		else if(strcasestr(buf, "Accept-Encoding:"))
		{
                        strcpy(buf, accept_encoding);
			flag[4] = 1;
                }

                if(!strcmp(buf, "\r\n") || !strcmp(buf, "\n"))
                        break;

                if(rio_writen(client_fd, buf, strlen(buf)) == -1)
                        return -1;

                //fprintf(stderr, "%s", buf);
        }

        /* 检查必要的请求报头，没有则发送 */
        if(!flag[0])
	{
                sprintf(buf, "Host: %s:%hu\r\n", host, port);
                if (rio_writen(client_fd, buf, strlen(buf)) == -1)
                        return -1;
                //fprintf(stderr, "%s", buf);
        }
        if(!flag[1])
	{
                strcpy(buf, connection_str);
                if (rio_writen(client_fd, buf, strlen(buf)) == -1)
                        return -1;
                //fprintf(stderr, "%s", buf);
        }
        if(!flag[2])
	{
                strcpy(buf, user_agent);
                if (rio_writen(client_fd, buf, strlen(buf)) == -1)
                        return -1;
                //fprintf(stderr, ("%s", buf);
        }
        if(!flag[3])
	{
                strcpy(buf, accept_str);
                if (rio_writen(client_fd, buf, strlen(buf)) == -1)
                        return -1;
                //fprintf(stderr, "%s", buf);
        }
        if(!flag[4])
	{
                strcpy(buf, accept_encoding);
                if (rio_writen(client_fd, buf, strlen(buf)) == -1)
                        return -1;
                //fprintf(stderr, "%s", buf);
        }

        /* 空行终止请求报头 */
        strcpy(buf, "\r\n");
        if(rio_writen(client_fd, buf, strlen(buf)) == -1)
                return -1;

        return 0;
}

/*
 * 从服务器读取响应，返回给浏览器，并且保存在缓存里
 * 首先检查状态值
 * 状态值正确了再读入服务器返回的响应报头响应主体
 * 并将其返回浏览器并且保存在网页缓存中
 */
int server2browser(rio_t *server_rp, int browser_fd, char *url){

        char buf[MAXLINE];
        char *ptr = NULL, *p = NULL;
        int tsize = 0, csize = 0, n = 0, empty = 0;
	/* cache总大小tsize， 响应主体的大小csize */
        
	/* 为缓存做准备 */
        p = ptr = malloc(MAX_OBJECT_SIZE);
        if(!p)
                return -1;

        /* 接受返回的第一行响应 */
        if((rio_readlineb(server_rp, buf, MAXLINE)) == -1)
	{
                if(ptr)
		{
                        Free(ptr);
                        ptr = NULL;
                }
                return -1;
        }

        /* 检查是否为空 */
        char sline[MAXLINE], *tmp, *tmp1;
        strcpy(sline, buf);

        tmp = strchr(sline, ' ') + 1;
        tmp1 = strchr(tmp, ' ');
        *tmp1 = '\0';
        /* 1开头为临时响应，需要请求者继续执行操作；204无内容，304请求的网页没有修改过 */
        if(*tmp == '1' || !strcmp(tmp, "204") || !strcmp(tmp, "304"))
                empty = 1;

        /* write ascii to browser and cache */
        if(buf2cache_browser(browser_fd, &tsize, &ptr, &p, buf, -1) == -1)
                return -1;

        /* 接受服务器响应报头，返回给浏览器 */
        while((n = rio_readlineb(server_rp, buf, MAXLINE)))
	{
                if(n == -1)
		{
                        if(ptr)
			{
                                Free(ptr);
                                ptr = NULL;
                        }
                        return -1;
                }

                /* 获得响应主体的长度 */
		if (!strncasecmp(buf, "Content-Length: ", 16))
          		 csize = atoi(buf + 16); 

                /* 写入cache和浏览器 */
                if(buf2cache_browser(browser_fd, &tsize, &ptr, &p, buf, -1) == -1)
                        return -1;

                if(!strcmp(buf, "\r\n"))
                        break;
        }

        
	//如果返回状态为空	
	if(empty)
	{ 
                if(ptr)
                        if(Create_cache(url, ptr, tsize) == -1)
                                return -1;
                return 0;
        }

        /* 接受服务器响应主体，返回浏览器 */
        if(csize == 0)
	{
                while((n = rio_readnb(server_rp, buf, MAXLINE)))
		{
                        if(n == -1)
			{
                                if(ptr)
				{
                                        Free(ptr);
                                        ptr = NULL;
                                }
                                return -1;
                        }

                        /* 写入cache和浏览器 */
                        if(buf2cache_browser(browser_fd, &tsize, &ptr, &p, buf, n) == -1)
                                return -1;
                }
        }
        else   /* 需要读入的长度不为0 */
        {
                if((tsize += csize) > MAX_OBJECT_SIZE)
                        /*  过大，结束 */
                        if(ptr)
			{
                                Free(ptr);
                                ptr = NULL;
                        }

                /* 大块读入 */
                while(csize >= MAXLINE)
		{
                        if((n = rio_readnb(server_rp, buf, MAXLINE)) == -1)
			{
                                if(ptr)
				{
                                        Free(ptr);
                                        ptr = NULL;
                                }
                                return -1;
                        }

                        /* 写入cache和浏览器 */
                        if(buf2cache_browser(browser_fd, NULL, &ptr, &p, buf, n) == -1)
                                return -1;

                        csize -= MAXLINE;
                }

                /* 读取剩余部分 */
                if(csize > 0)
		{
                        if((n = rio_readnb(server_rp, buf, csize)) == -1)
			{
                                if(ptr)
				{
                                        Free(ptr);
                                        ptr = NULL;
                                }
                                return -1;
                        }

                        /* 写入cache和浏览器 */
                        if(buf2cache_browser(browser_fd, NULL, &ptr, &p, buf, n) == -1)
                                return -1;
                }
        }
        
        /* 创建缓存区，保存网页 */
        if(ptr)
                if(Create_cache(url, ptr, tsize) == -1)
                        return -1;

        return 0;
}

/*
 * parse_url：解析url
 */
static int parse_url(const char *aurl, char *uri, char *host, unsigned short *portp){
        char url[MAXLINE], *tmp, *tmp1, *tmp2;

        if(!strcmp(aurl, "*"))   
                return -1;

        strcpy(url, aurl);

        /* host */
        tmp = strstr(url, "://"); 
        if(!tmp)     
                tmp = url;
        else
                tmp += 3;

        /* uri */
        tmp1 = tmp2 = strchr(tmp, '/');
        if(!tmp1)
                strcpy(uri,"/");
        else
	{
                while(*(tmp2 + 1) == '/')
                        tmp2 += 1;
                strcpy(uri, tmp2);
                *tmp1 = '\0';
        }

        /* host[:port] */
        if(index(tmp, ':'))
                sscanf(tmp, "%[^':']:%hu", host, portp);
        else
	{
                strcpy(host, tmp);
                *portp = 80;
        }

        return 0;
}

/*
 * 将buf写入cache和浏览器（length = -1时表示不知buf大小）
 * tsizep: 目前cache的总大小，NULL时表示不知cache大小
 * cache: 指向cache
 * pp: cache的索引
 */
static int buf2cache_browser(int fd, int *tsizep, char **cachep, char **pp, char *buf, int length)
{
        if (length == -1)
		length = strlen(buf);
        int n = rio_writen(fd, buf, length);
        if(n == -1)
	{
                if(*cachep)
		{
                        Free(*cachep);
                        *cachep = NULL;
                }
                return -1;
        }
        if(*cachep)
	{
		if (tsizep)
		{                
			*tsizep += n;
		        if(*tsizep > MAX_OBJECT_SIZE)
			{
		                if(*cachep)
				{
		                        Free(*cachep);
		                        *cachep = NULL;
		                }
		        }
			else
			{
		                memcpy(*pp, buf, n);
		                *pp += n;
		        }
		}
		else
		{
			 memcpy(*pp, buf, n);
                	*pp += n;
		}
        }

        return 0;
}
