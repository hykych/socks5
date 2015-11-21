#include "libclass.h"

Conn::~Conn()
{
    free(w_client);
    free(w_serv);
}

template<typename First,typename Second> 
Map<First,Second>::Map()
{
    M.clear();
    CS_INIT(&lock);
}

template<typename First,typename Second>
Map<First,Second>::~Map()
{
    CS_ENTER(&lock);
    M.clear();
    CS_LEAVE(&lock);
}

template<typename First,typename Second>
Second Map<First,Second>::find(const First& __x)
{
    Second ans;
    CS_ENTER(&lock);
    if( M.find(__x) != M.end()){
	ans = M[__x];
    }
    else ans = 0;
    CS_LEAVE(&lock);

    return ans;
}

template<typename First,typename Second>
void Map<First,Second>::insert(const First& __x,const Second& __y)
{
    CS_ENTER(&lock);
    M[__x] = __y;
    CS_LEAVE(&lock);
}

template<typename First,typename Second>
void Map<First,Second>::erase(const First& __x)
{
    CS_ENTER(&lock);
    M.erase(__x);
    CS_LEAVE(&lock);
}

