#ifndef ICOMET_CHANNEL_H
#define ICOMET_CHANNEL_H

#include <string>
#include <evhttp.h>

class Server;
class Channel;

class Subscriber{
public:
	Subscriber *prev;
	Subscriber *next;
	
	Server *serv;
	Channel *channel;
	struct evhttp_request *req;
	std::string cb;
	int idle;
};

class Channel{
public:
	int id;
	int sub_count;
	Subscriber *subs;
	// TODO: msg_list
	int seq_send;
	
	Channel();
	~Channel();
	void add_subscriber(Subscriber *sub);
	void del_subscriber(Subscriber *sub);
};

#endif
