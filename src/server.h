#ifndef ICOMET_SERVER_H
#define ICOMET_SERVER_H

#include "../config.h"
#include <vector>
#include <evhttp.h>
#include <event2/http.h>
#include "channel.h"
#include "util/objpool.h"

#define DEFAULT_JSONP_CALLBACK	"icomet_cb"
#define CHANNEL_CHECK_INTERVAL	3
#define CHANNEL_IDLE_TIMEOUT	12
#define CHANNEL_MAX_IDLES		(CHANNEL_IDLE_TIMEOUT/CHANNEL_CHECK_INTERVAL)
#define SUB_IDLE_TIMEOUT		12 // in second s
#define SUB_MAX_IDLES			(SUB_IDLE_TIMEOUT/CHANNEL_CHECK_INTERVAL)

class Server{
private:
	std::vector<Channel> channel_slots;
	ObjPool<Subscriber> sub_pool;
	int max_channels;
	int max_subscribers_per_channel;
	
	struct{
		int size;
		Channel *head;
		Channel *tail;
	}channels;
	
	void add_channel(Channel *channel);
	void del_channel(Channel *channel);
public:
	enum{
		AUTH_NONE = 0,
		AUTH_TOKEN = 1
	};

	int auth;
	
	Server(int max_channels, int max_subscribers_per_channel);
	~Server();
	
	int sub(struct evhttp_request *req);
	int sub_end(Subscriber *sub);
	int ping(struct evhttp_request *req);

	int pub(struct evhttp_request *req);
	int sign(struct evhttp_request *req);
	int check_timeout();
};

#endif
