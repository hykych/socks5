#include "socks5.h"

static int g_sockfd = 0;

struct ev_loop *g_loop;

struct ev_io g_io_accept;

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
	PRINTF(LEVEL_ERROR,"setsockopt SO_REUSEADDR fail.\n");
	return R_ERROR;
    }

    if(bind( sockfd, (struct sockaddr *)&serv, sizeof(serv)) < 0)
    {
	PRINTF( LEVEL_ERROR,"bind error[%d]\n",errno);
	return R_ERROR;
    }

    if(listen( sockfd, backlog) < 0)
    {
	PRINTF( LEVEL_ERROR,"listen error.\n");
	return R_ERROR;
    }
    return sockfd;
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
	read = recv( watcher->fd, buf,BUF_SIZE, 0);
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
	    temp = send( ((ev_io*)watcher->data)->fd, buf+write, read, 0);

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


static INT32 socks5_sockset(int sockfd)
{
    int opt = 1;
    int flags;
    /*
    struct timeval tmo;

    tmo.tv_sec = 5;
    tmo.tv_usec = 0;
    
    if( -1 == setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char *)&tmo, sizeof(tmo))
	    ||setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,(char *)&tmo, sizeof(tmo))){
	PRINTF(LEVEL_ERROR,"setsockopt timeo error.\n");
	return R_ERROR;
    }

    if( -1 == setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR, &opt, sizeof(opt))){
	PRINTF(LEVEL_ERROR,"setsockopt SO_REUSEADDR error.\n");
	return R_ERROR;
    }
    */
    if( -1 == (flags = fcntl(sockfd, F_GETFL, 0)))
	flags = 0;

    fcntl( sockfd, F_SETFL, flags | O_NONBLOCK);

}

static INT32 socks5_auth(int sockfd)
{
    int remote = 0;
    char buf[BUF_SIZE];
    struct sockaddr_in addr;
    int addr_len;
    int ret;

    //socks5_sockset(sockfd);

    if( -1  ==  recv( sockfd, buf, 2, MSG_WAITALL)) {
	PRINTF(LEVEL_ERROR,"recv error.\n");
	goto err;
    }
    if(SOCKS5_VERSION != ((socks5_method_req_t*)buf)->ver) {
	PRINTF(LEVEL_ERROR,"socks5 version error.\n");
	goto err;
    }
    ret = ((socks5_method_req_t *)buf)->nmethods;
    if(-1 == recv( sockfd, buf, ret, 0)){
	PRINTF(LEVEL_ERROR,"recv error.\n");
	goto err;
    }

    memcpy(buf,"\x05\x00",2);
    if( -1 == send(sockfd, buf, 2, 0)){
	PRINTF(LEVEL_ERROR,"send error.\n");
	goto err;
    }

    if(-1 == recv(sockfd, buf, 4, 0)) {
	PRINTF(LEVEL_ERROR,"recv error.\n");
	goto err;
    }
    
    if(SOCKS5_VERSION != ((socks5_request_t *)buf)->ver
	    || SOCKS5_CMD_CONNECT != ((socks5_request_t *)buf)->cmd)
    {
	PRINTF(LEVEL_DEBUG,"ver : %d\tcmd = %d.\n",
		((socks5_request_t *)buf)->ver,((socks5_request_t *)buf)->cmd);
	((socks5_response_t *)buf)->ver = SOCKS5_VERSION;
	((socks5_response_t *)buf)->cmd = SOCKS5_CMD_NOT_SUPPORTED;
	((socks5_response_t *)buf)->rsv = 0;

	goto err;
    }

    if(SOCKS5_IPV4 == ((socks5_request_t *)buf)->atype)
    {
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;

	if( -1 == recv(sockfd, buf, 4, 0)){
	    PRINTF(LEVEL_ERROR,"recv error.\n");
	    goto err;
	}
	memcpy(&(addr.sin_addr.s_addr), buf, 4);
	if( -1 == recv(sockfd, buf, 2, 0)) {
	    PRINTF(LEVEL_ERROR,"recv error.\n");
	    goto err;
	}
	memcpy( &(addr.sin_port), buf, 2);

	//PRINTF(LEVEL_DEBUG, "type : IP, %s:%d.\n",inet_ntoa(addr.sin_addr),htons(addr.sin_port));
    }
    else if(SOCKS5_DOMAIN == ((socks5_request_t *)buf)->atype)
    {
	struct hostent *hptr;

	memset(&addr,0,sizeof(addr));

	if(-1 == recv( sockfd, buf, 1, 0)) {
	    PRINTF(LEVEL_ERROR,"recv error.\n");
	    goto err;
	}
	ret = buf[0];
	buf[ret] = 0;
	if(-1 == recv( sockfd, buf, ret, 0)) {
	    PRINTF(LEVEL_ERROR,"recv error.\n");
	    goto err;
	}

	hptr = gethostbyname(buf);
	
	PRINTF( LEVEL_DEBUG, "type : domain [%s].\n",buf);

	if(NULL == hptr) goto err;
	if(AF_INET !=hptr->h_addrtype) goto err;
	if(NULL == *(hptr->h_addr_list)) goto err;

	addr.sin_family = AF_INET;
	memcpy( &(addr.sin_addr.s_addr),  *(hptr->h_addr_list), 4);

	if(-1 == recv( sockfd, buf, 2, 0)) goto err;
	memcpy( &(addr.sin_port), buf, 2);

    }
    else
    {
	((socks5_response_t *)buf)->ver = SOCKS5_VERSION;
	((socks5_response_t *)buf)->cmd = SOCKS5_ADDR_NOT_SUPPORTED;
	((socks5_response_t *)buf)->rsv = 0;

	send( sockfd, buf, 4, 0);
	goto err;
    }

    if((remote = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	PRINTF(LEVEL_ERROR,"socket error.\n");
	goto err;
    }
    //socks5_sock
    if(0 > connect(remote,(struct sockaddr *)&addr, sizeof(addr)))
    {
	PRINTF(LEVEL_ERROR, "connect error.\n");
        
	//connect error
	memcpy(buf,"\x05\x05\x00\x01\x00\x00\x00\x00\x00\x00",10);
	send(sockfd, buf, 4, 0);

	goto err;
    }

    addr_len = sizeof(addr);
    if( 0 > getpeername(remote,(struct sockaddr *)&addr,(socklen_t *)&addr_len)){
	PRINTF(LEVEL_ERROR,"getpeername[%d] error.\n",errno);
	goto err;
    }
    
    memcpy(buf, "\x05\x00\x00\x01",4);
    memcpy(buf + 4, &(addr.sin_addr.s_addr), 4);
    memcpy(buf + 8, &(addr.sin_port), 2);
    send(sockfd, buf, 10, 10);

    PRINTF(LEVEL_DEBUG,"auto ok.\n");
    return remote;

err:
    if(0 != remote) close(remote);
    return R_ERROR;
}

void* new_custom(void *para)
{
    struct ev_io* w_client = (struct ev_io *)para, *w_serv = (struct ev_io *)w_client->data;
    int client_fd = w_client->fd;
    int remote_fd;
    int opt = 1;
    struct ev_loop* loop = nullptr;

    if(!(loop = ev_loop_new(0))){
	PRINTF(LEVEL_ERROR,"ev_loop_new fail.\n");
	goto err;
    }

    if(R_ERROR == (remote_fd = socks5_auth(client_fd))){
	PRINTF(LEVEL_ERROR,"socks5 auth error.\n");
	goto err;
    }

    socks5_sockset(remote_fd);
    socks5_sockset(client_fd);

    ev_io_init(w_serv,read_cb,remote_fd,EV_READ);

    ev_io_start(loop,w_serv);
    ev_io_start(loop,w_client);

    ev_run(loop,0);

    close(client_fd);
    close(remote_fd);

    ev_io_stop( loop, w_serv);
    ev_io_stop( loop, w_client);

err:
    free(w_client);
    free(w_serv);
    if(loop != nullptr) ev_loop_destroy(loop);
    return nullptr;
}


static void accept_cb(struct ev_loop* loop,ev_io* watcher,INT32 revents)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd;

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

    THREAD_CREATE(new_custom,(void *)w_client);

    return;
err:
    free(w_client);
    free(w_serv);

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
    g_cfg.port = 9999;

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
    signal(SIGPIPE,SIG_IGN);
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


