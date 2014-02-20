#ifndef ICOMET_SUBSCRIBER_H
#define ICOMET_SUBSCRIBER_H

#include <string>
#include <evhttp.h>

class Server;
class Channel;


class Subscriber{
public:
	enum Type{
		POLL	= 0,
		STREAM	= 1
	};
public:
	Subscriber *prev;
	Subscriber *next;
	
	Server *serv;
	Channel *channel;
	std::string callback;
	int type;
	int idle;
	// 当类型是 data 时的 seq, 和类型是 noop 时用的不是同一套 seq.
	int seq;
	// noop_seq 用于 js long-polling 判断重复连接
	int noop_seq;
	struct evhttp_request *req;

	void start();
	void close();
	void send_chunk(const char *type, const char *content);
	
	void noop();
	void send_old_msgs();
};

#endif
