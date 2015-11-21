#include "socks5.h"

static int g_sockfd = 0;

struct ev_loop *g_loop;

struct ev_io g_io_accept;

Map< ev_timer*,ev_tstamp> M;

socks5_cfg_t g_cfg;


static int socks5_srv_init(UINT16 port,INT32 backlog)
{
    sockaddr_in serv;
    int sockfd;
    int opt;
    int flags;

    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
	PRINTF(LEVEL_ERROR,"socket error!\n");
	return R_ERROR;
    }

    memset(&serv,0,sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(port);

    if( -1 == (flags = fcntl(sockfd, F_GETFL, 0)))
	flags = 0;

    fcntl( sockfd, F_SETFL, flags | O_NONBLOCK);

    opt = 1;
    if( -1 == setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
	PRINTF(LEVLE_ERROR,"setsockopt SO_REUSEADDR fail.\n");
	return R_ERROR;
    }

    //if(-1 == setsockopt(sockfd,SOL_SOCKET,SO_NOSIGPIPE,&opt,sizeof(opt)))
    //{
    //	PRINTF(LEVEL_ERROR,"setsockopt SO_NOSIGPIPE
    //   }


    if(bind( sockfd, (sockaddr *)&serv, sizeof(serv)) < 0)
    {
	PRINTF( LEVEL_ERROR,"bind error[%d]\n",errno);
	return R_ERROR;
    }

    if(listen( sockfd, backlog) < 0)
    {
	PRINTF( LEVEL_ERROR,"listen error.\n");
	return R_ERROR;
    }
    return R_OK;
}

static void tm_cb(struct ev_loop* loop, struct ev_timer* watcher,int revents)
{
    ev_tstamp last_activity = M.find(watcher);

    ev_tstamp after = last_activity - ev_time() + 600;

    if(after < 0)
    {
	ev_break(loop,EVBREAK_ALL);
    }
    else {
	watcher->repeat = after;
	ev_timer_again(loop,watcher);
    }
    return;
}

static void read_cb(struct ev_loop* loop, struct ev_io* watcher,int revents)
{
    char buf[BUF_SIZE];
    int read, write, temp;
    if(EV_ERROR & revents)
    {
	PRINTF(LEVEL_ERROR,"error event in read.\n");
	goto err;
    }

    while(true){
	read = recv(watcher->fd, buf,BUF_SIZE, 0);
	if(read == 0) {
	    goto err;
	}
	if(read < 0 ){
	    if(errno == EAGAIN) break;
	    if(errno == EINTR)  continue;
	    goto err;
	}

	write = 0;
	while(read)
	{
	    temp = send(((ev_io*)watcher->data)->fd,buf+write,read,0);

	    if(temp == 0){
		goto err;
	    }
	    if(temp < 0){
		if(errno == EAGAIN) continue;
		if(errno == EINTR) continue;
		goto err;
	    }
	    read  -= temp;
	    write += temp;
	}
    }
    return;
err:
    ev_break(loop,EVBREAK_ALL);
    return;
}

void* new_custom(void *para)
{
    Conn* work = (Conn *)para;
    int client_fd = work->w_client->fd;
    int remote_fd;
    int opt = 1;
    struct ev_loop* loop = nullptr;
    struct ev_timer  timer_watcher;
    if(!(loop = ev_loop_new(0))){
	PRINTF(LEVLE_ERROR,"ev_loop_new fail.\n");
	goto err;
    }

    if(R_ERROR == (remote_fd = socks5_auth(client_fd))){
	PRITF(LEVEL_ERROR,"socks5 auth error.\n");
	goto err;
    }

    setsockopt( remote_fd, SOL_SOCKET, O_NONBLOCK, &opt, sizeof(opt));
    setsockopt( client_fd, SOL_SOCKET, O_NONBLOCK, &opt, sizeof(opt));

    ev_io_init(work->w_serv,read_cb,remote_fd,EV_READ);

    ev_io_start(loop,work->w_serv);
    ev_io_start(loop,work->w_client);

    ev_init(&timer_watcher,tm_cb);
    timer_watcher.repeat = 600;
    ev_timer_again(loop,&timer_watcher);

    M.insert( &timer_watcher, ev_time());

    ev_run(loop,0);

    close(client_fd);
    close(remote_fd);
    ev_io_stop( loop, work->w_serv);
    ev_io_stop( loop, work->w_client);
    ev_timer_stop(loop, &timer_watcher);

    M.erase(&timer_watcher);
err:
    delete work;
    if(loop != nullptr) ev_loop_destroy(loop);

    return nullptr;
}


static void accept_cb(struct ev_loop* loop,ev_io* watcher,INT32 revents)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd;
    int remote_fd;
    Conn* para = nullptr;
    struct ev_io *w_client = nullptr, *w_serv = nullptr;

    if(EV_ERROR & revents)
    {
	PRINTF(LEVEL_ERROR, "error event in accept.\n");
	goto err;
    }

    w_client = (struct ev_io*) malloc(sizeof(struct ev_io));
    w_serv   = (struct ev_io*) malloc(sizeof(struct ev_io));

    if(!w_client || !w_serv)
    {
        PRINTF(LEVEL_ERROR,"apply memory fail.\n");
	goto err;
    }

    client_fd = accept( watcher->fd, (struct sockaddr *)&client_addr, &client_len);

    if(client_fd < 0)
    {
	PRINTF(LEVEL_ERROR,"accept fail.\n");
	goto err;
    }
    w_client->data = w_serv;
    w_serv->data   = w_client;

    ev_io_init(w_client,read_cb,client_fd,EV_READ);

    try{
	para = new Conn;
    }
    catch(const std::bad_alloc& bad)
    {
	PRINTF(LEVEL_ERROR,"apply memory fail.\n");
	para = nullptr;
	goto err;
    }

    para->w_client = w_client;
    para->w_serv   = w_serv;

    THREAD_CREATE(new_custom,(void *)para);

    return;
err:
    free(w_client);
    free(w_serv);
    delete para;
    return;
}


static void help()
{
    printf("Usage: socks5 [options]\n");
    printf("Options:\n");
    printf("    -p <port>       tcp listen port\n");
    //printf("    -d <Y|y>        run as a daemon if 'Y' or 'y', otherwise not\n");

    //printf("    -l <level>      debug log level,range [0, 5]\n");
    printf("    -h              print help information\n");
}



//解析参数，到时补充
static INT32 check_para(INT32 argc, char* argv[])
{
    char ch;
    memset(&g_cfg,0,sizeof(g_cfg));
    g_cfg.port = 8888;

    while( (ch = getopt(argc,argv,":p:h")) != -1)
    {
	switch(ch)
	{
	    case 'p':
		g_cfg.port = atoi(optarg);
		break;
	    case 'h':
		help();
		exit(0);
	}
    }

    return R_OK;
}

//信号处理，到时补充
static void signal_init()
{
}

int main(int argc,char *argv[])
{
    if(R_ERROR == check_para(argc,argv))
    {
	PRINTF(LEVEL_ERROR,"check argument error.\n");
	return R_ERROR;
    }

    signal_init();

    PRINTF(LEVEL_INFORM,"socks5 staring ...\n");

    if((g_sockfd = socks5_srv_init(g_cfg.port,2000)) == R_ERROR)
    {
	PRINTF(LEVEL_ERROR, "socks server init error.\n");
	return R_ERROR;
    }

    g_loop = ev_default_loop(0);

    ev_io_init(&g_io_accept,accept_cb,g_sockfd,EV_READ);

    ev_io_start(g_loop,&g_io_accept);

    ev_run(g_loop, 0);

    PRINTF(LEVEL_INFORM,"time to exit.\n");

    //socks_srv_exit(g_sockfd);

    PRINTF(LEVEL_INFORM,"exit socket server.\n");

    return 0;
}


