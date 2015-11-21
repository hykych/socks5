#include <iostream>
#include <cstdio>
#include <ev.h>
#include "libunp/unp.h"
#include "libthread/libthread.h"
#include "./inc/def.h"
#include "./libclass/libclass.h"
//#include "socks5.h"

#define SOCKS5_STATE_PREPARE 0
#define SOCKS5_STATE_RUNNING 1
#define SOCKS5_STATE_STOP 2


static void signal_init();

static INT32 check_para(int argc,char *argv[]);

static INT32 socks_srv_init(UINT16 port,INT32 backlog);

static void accept_cb(struct ev_loop *loop, struct ev_io* watcher, int revents);

static void read_cb(struct ev_loop *loop, struct ev_io* watcher, int revents);


