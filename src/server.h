#ifndef ICOMET_SERVER_H
#define ICOMET_SERVER_H

#include <vector>
#include <evhttp.h>
#include <event2/http.h>
#include "channel.h"
#include "util/objpool.h"

#define DEFAULT_JSONP_CALLBACK "icomet_cb"

class Server{
private:
	std::vector<Channel> channels;
	ObjPool<Subscriber> sub_pool;
public:
	Server();
	~Server();
	
	int sub(struct evhttp_request *req);
	int pub(struct evhttp_request *req);
	int sub_end(Subscriber *sub);
	int heartbeat();
};

#endif
