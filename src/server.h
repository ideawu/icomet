#ifndef ICOMET_SERVER_H
#define ICOMET_SERVER_H

#include "../config.h"
#include <vector>
#include <evhttp.h>
#include <event2/http.h>
#include "channel.h"
#include "util/objpool.h"

#define DEFAULT_JSONP_CALLBACK	"icomet_cb"
#define MAX_CHANNELS			1000000
#define MAX_SUBS_PER_CHANNEL	16
#define SUB_CHECK_INTERVAL		3
#define SUB_IDLE_TIMEOUT		12
#define SUB_MAX_IDLES			(SUB_IDLE_TIMEOUT/SUB_CHECK_INTERVAL)

class Server{
private:
	std::vector<Channel> channel_slots;
	ObjPool<Subscriber> sub_pool;
	
	struct{
		int size;
		Channel *head;
		Channel *tail;
	}channels;
	
	void add_channel(Channel *channel);
	void del_channel(Channel *channel);
	
	void channel_send(Channel *channel, const char *type, const char *content);
public:
	Server();
	~Server();
	
	int sub(struct evhttp_request *req);
	int sub_end(Subscriber *sub);
	int ping(struct evhttp_request *req);

	int pub(struct evhttp_request *req);
	int check_timeout();
};

#endif
