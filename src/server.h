#ifndef ICOMET_SERVER_H
#define ICOMET_SERVER_H

#include <vector>
#include <map>
#include <list>
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
	ObjPool<Subscriber> sub_pool;
	std::vector<Channel> channel_slots;
	// mapping obj to channel
	std::map<std::string, Channel *> obj_channels;
	std::list<Channel *> free_channels;
	
	struct{
		int size;
		Channel *head;
		Channel *tail;
	}channels;

	int subscribers;
	
	Channel* get_channel(int cid);
	Channel* get_channel_by_obj(const std::string &obj);
	void add_channel(Channel *channel);
	void del_channel(Channel *channel);
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
