#ifndef ICOMET_CHANNEL_H
#define ICOMET_CHANNEL_H

#include "../config.h"
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
	std::string callback;
	int idle;
	int noop_seq;
};

class Channel{
public:
	Channel *prev;
	Channel *next;

	struct{
		int size;
		Subscriber *head;
		Subscriber *tail;
	}subs;

	int id;
	int seq;
	// TODO: msg_list
	
	Channel();
	~Channel();
	void add_subscriber(Subscriber *sub);
	void del_subscriber(Subscriber *sub);
};

#endif
