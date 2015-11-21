#ifndef _LIBCLASS_H
#define _LIBCLASS_H

#include "../inc/def.h"
#include "../libunp/unp.h"
#include "../libthread/libthread.h"
#include <map>

struct socks5_cfg_t
{
    UINT16 port;
};

struct Conn{
    struct ev_io *w_client,*w_serv;
    ~Conn();
};


template <typename First,typename Second> 
class Map
{
    private:
	std::map<First,Second> M;
	CS_T lock;
    public:
	Map();
	~Map();
	void erase(const First&);
	Second find(const First&);
	void insert(const First&,const Second&);
};


#endif

