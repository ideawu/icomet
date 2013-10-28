#ifndef ICOMET_SERVER_H
#define ICOMET_SERVER_H

#include <vector>
#include <map>
#include <list>
#include <evhttp.h>
#include <event2/http.h>
#include "channel.h"
#include "util/list.h"
#include "util/objpool.h"

#define DEFAULT_JSONP_CALLBACK	"icomet_cb"
#define CHANNEL_CHECK_INTERVAL	3

class Server{
private:
	ObjPool<Subscriber> sub_pool;
	std::vector<Channel> channel_slots;
	// mapping cname(channel_name) to channel
	std::map<std::string, Channel *> cname_channels;
	
	LinkedList<Channel *> used_channels;
	LinkedList<Channel *> free_channels;

	int subscribers;
	
	Channel* get_channel(int cid);
	Channel* get_channel_by_name(const std::string &name);
	Channel* new_channel(const std::string &cname);
	void free_channel(Channel *channel);
public:
	enum{
		AUTH_NONE = 0,
		AUTH_TOKEN = 1
	};

	int auth;
	
	Server();
	~Server();
	
	int check_timeout();
	
	int sub(struct evhttp_request *req);
	int sub_end(Subscriber *sub);
	int ping(struct evhttp_request *req);

	int pub(struct evhttp_request *req);
	int sign(struct evhttp_request *req);
	int close(struct evhttp_request *req);
	int info(struct evhttp_request *req);
	int check(struct evhttp_request *req);
};

#endif
