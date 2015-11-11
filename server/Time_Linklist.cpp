#include "Time_Linklist.h"


LinkNode::LinkNode(LinkNode *_prev, LinkNode *_next,ev_tstamp t)
{
    pthread_mutex_init(&lock,0);
    prev = _prev,next = _next;
    Time = t;
}

LinkNode::~LinkNode()
{
    /*
    ::close(serv_watcher->fd);
    ::close(cli_watcher->fd);
    serv_watcher->stop();
    cli_watcher->stop();
    */
}

void LinkNode::Close()
{
    pthread_mutex_lock(&lock);

    if(serv_watcher->fd != -1) ::close(serv_watcher->fd);
    if( cli_watcher->fd != -1) ::close(cli_watcher->fd);

    pthread_mutex_unlock(&lock);
}

void LinkNode::CleanUp()
{
    this->next->prev = this->prev;
    this->prev->next = this->next;

    Close();
    serv_watcher->stop();
    cli_watcher->stop();
}

Time_Linklist::Time_Linklist()
{
    head       = new LinkNode(NULL,NULL);
    tail       = new LinkNode(NULL,NULL);
    head->next = tail;
    tail->prev = head;
    pthread_mutex_init(&lock,0);
}


Time_Linklist::~Time_Linklist()
{
    while(head->next != tail) {
	LinkNode *temp = head->next;

	temp->CleanUp();
	delete temp;
    }
    delete head;
    delete tail;
}

void Time_Linklist::Lock()
{
    pthread_mutex_lock(&lock);
}

void Time_Linklist::Unlock()
{
    pthread_mutex_unlock(&lock);
}


void Time_Linklist::Remove_Timeout()
{
    Lock();

    ev_tstamp t = ev_time();
    while(head->next != tail && t - head->next->Time > 600)
    {
	LinkNode *temp = head->next;

	temp->CleanUp();
	delete temp;
    }

    Unlock();
}

void Time_Linklist::Totail(LinkNode *Node)
{
    Lock();

    Node->next->prev = Node->prev;
    Node->prev->next = Node->next;

    tail->prev->next = Node;
    Node->prev       = tail->prev;
    tail->prev       = Node;
    Node->next       = tail;
    Node->Time       = ev_time();

    Unlock();
}


void Time_Linklist::Insert(LinkNode *Node)
{
    Lock();

    tail->prev->next = Node;
    Node->prev       = tail->prev;
    tail->prev       = Node;
    Node->next       = tail;
    Node->Time       = ev_time();

    Unlock();
}
