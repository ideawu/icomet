#ifndef ICOMET_CHANNEL_H
#define ICOMET_CHANNEL_H

#include <vector>

class Channel;

class Subscriber{
public:
	Subscriber *prev;
	Subscriber *next;
	
	struct evhttp_request *req;
	int last_recv;
	Channel *channel;
	void *ptr;
};

class Channel{
public:
	int id;
	int sub_count;
	Subscriber *subs;
	// TODO: msg_list
	
	Channel();
	~Channel();
	void add_subscriber(Subscriber *sub);
	void del_subscriber(Subscriber *sub);
};

#endif
