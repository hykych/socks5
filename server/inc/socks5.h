#include <iostream>
#include <cstdio>
#include <ev.h>
#include "unp.h"
#include "libthread.h"
#include "def.h"
#include "liblog.h"
//#include "socks5.h"

#define SOCKS5_STATE_PREPARE 0
#define SOCKS5_STATE_RUNNING 1
#define SOCKS5_STATE_STOP 2

#define SOCKS5_VERSION 0x05
#define SOCKS5_CMD_CONNECT 0x01
#define SOCKS5_CMD_NOT_SUPPORTED 0x07
#define SOCKS5_ADDR_NOT_SUPPORTED 0x08

#define SOCKS5_IPV4 0x01
#define SOCKS5_DOMAIN 0X03

struct socks5_method_req_t
{
    UINT8 ver;
    UINT8 nmethods;
};

struct socks5_request_t
{
    UINT8 ver;
    UINT8 cmd;
    UINT8 rsv;
    UINT8 atype;
};

typedef socks5_request_t socks5_response_t;


static void signal_init();

static INT32 check_para(int argc,char *argv[]);

static INT32 socks_srv_init(UINT16 port,INT32 backlog);

static void accept_cb(struct ev_loop *loop, struct ev_io* watcher, int revents);

static void read_cb(struct ev_loop *loop, struct ev_io* watcher, int revents);


