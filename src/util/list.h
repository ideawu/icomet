#ifndef UTIL_LIST_H
#define UTIL_LIST_H

#define list_reset(L)\
	do{\
		L.size = 0;\
		L.head = NULL;\
		L.tail = NULL;\
	}while(0)

#define list_add(L, i)\
	do{\
		(L).size ++;\
		(i)->prev = (L).tail;\
		(i)->next = NULL;\
		if((L).tail){\
			(L).tail->next = (i);\
		}else{\
			(L).head = (i);\
		}\
		(L).tail = (i);\
	}while(0)

#define list_del(L, i)\
	do{\
		(L).size --;\
		if((i)->prev){\
			(i)->prev->next = (i)->next;\
		}\
		if((i)->next){\
			(i)->next->prev = (i)->prev;\
		}\
		if((L).head == (i)){\
			(L).head = (i)->next;\
		}\
		if((L).tail == (i)){\
			(L).tail = (i)->prev;\
		}\
	}while(0)

#endif
