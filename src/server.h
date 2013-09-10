#ifndef ICOMET_SERVER_H
#define ICOMET_SERVER_H

#include <vector>
#include <event2/http.h>
#include "channel.h"
#include "util/objpool.h"

class Server{
private:
	std::vector<Channel> channels;
	ObjPool<Subscriber> sub_pool;
public:
	Server();
	~Server();
	
	int sub(struct evhttp_request *req);
	int pub(struct evhttp_request *req);
	int disconnect(Subscriber *sub);
};

#endif
