#ifndef ICOMET_CHANNEL_H
#define ICOMET_CHANNEL_H

#include "../config.h"
#include <string>
#include <vector>
#include <evhttp.h>

#define CHANNEL_MSG_LIST_SIZE	10

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
	int idle;

	int msg_seq_max;
	std::vector<std::string> msg_list;
	
	Channel();
	~Channel();
	void reset();
	void add_subscriber(Subscriber *sub);
	void del_subscriber(Subscriber *sub);
	void send( const char *type, const char *content);
};

#endif
