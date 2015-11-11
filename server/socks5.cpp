#include <iostream>
#include <cstdio>
#include <ev.h>
#include "unp.h"
#include "socks5.h"


static int g_sockfd = 0;
static socks5_cfg_t g_cfg = {0};
struct ev_loop* g_loop = NULL;
struct ev_io g_io_accept;


static INT32 socks5_srv_init(UINT16 port,INT32 backlog);
static INT32 socks5_srv_exit(int sockfd);

static INT socks5_sockset(int sockfd);

static void accept_cb(struct ev_loop* loop,struct ev_io* watcher,int revents);

static void read_cb(struct ev_loop* loop,struct ev_io* watcher,int revents);

static void timer_cb(struct ev_loop* loop, struct ev_timer* watcher,int revents);


static int socks5_srv_init(UINT16 port,INT32 backlog)
{
    struct sockaddr_in serv;
    int sockfd;
    int opt;
    int flags;

    if((sockfd = socket(AF_INET,SOCK_TREAM,0)) < 0){
	return R_ERROR;
    }

    memset(&serv,0,sizeof(serv));


    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(port);

    if(-1 == (flags = fcntl(sockfd,F_GETFL,0)))
	flags = 0;
    fcntl(sockfd,F_SETFL,flags | O_NONBLOCK);

    opt = 1;

    if(-1 == setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))){
	return R_ERROR;
    }
    

    if(bind(sockfd,(struct sockaddr *)&serv,sizeof(serv)) < 0){
	return R_ERROR;
    }

    if(listen(sockfd,backlog) < 0)
    {
	retrun R_ERROR;
    }

    return sockfd;
}

int main()
{
    singal_init();

    g_sockfd = socks5_srv_init(g_cfg.port,10);
    if(R_ERROR == g_sockfd)
    {
	return R_ERROR;
    }

    g_state = SOCKS5_STATE_RUNNING;

    g_loop = ev_default_loop(0);

    ev_io_init(&g_io_accept,accept_cb,g_sockgd,EV_READ);
    ev_io_start(g_loop,g_io_accept);

    ev_run(g_loop,0);
}
