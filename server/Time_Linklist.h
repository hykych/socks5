#include <pthread.h>
#include <ev++.h>
#include <memory>
#include "unp.h"

//using namespace ev;


//一个双向链表节点包含前指针和后指针，一个时间戳，两个指向监视器的智能指针
struct LinkNode{
    LinkNode *prev,*next;
    ev_tstamp Time; //时间戳
    pthread_mutex_t lock;//节点锁，用于互斥地关闭套接字
    std::auto_ptr<ev::io> serv_watcher, cli_watcher;

    LinkNode (LinkNode *_prev,LinkNode *_next,ev_tstamp t = 0);
    ~LinkNode();

    void CleanUp();//做析构函数之前的准备，把节点从链表中移除，stop监视器，关闭套接字
    void Close();//关闭双方套接字
};

class Time_Linklist
{
    private:
	pthread_mutex_t lock;
	LinkNode *head, *tail;
    public:

	Time_Linklist();
	~Time_Linklist();

	void Lock();
	void Unlock();

	//把过时的节点从链表中移除
	void Remove_Timeout();

	//把一个节点移到链表的尾部
	void Totail(LinkNode *Node);

	//把一个节点插入到链表的尾部
	void Insert(LinkNode *Node);
};
