#include <evhttp.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include "server.h"
#include "util/log.h"

#define MAX_CHANNELS 100000

Server::Server(){
	channels.resize(MAX_CHANNELS);
	for(int i=0; i<channels.size(); i++){
		Channel &ch = channels[i];
		ch.id = i;
	}
}

Server::~Server(){
}

static void on_disconnect(struct evhttp_connection *evcon, void *arg){
	Subscriber *sub = (Subscriber *)arg;
	Server *serv = (Server *)sub->ptr;
	serv->disconnect(sub);
}

int Server::disconnect(Subscriber *sub){
	Channel *channel = sub->channel;
	channel->del_subscriber(sub);
	sub_pool.free(sub);
	log_debug("channel: %d, del sub, sub_count: %d", channel->id, channel->sub_count);
	return 0;
}

int Server::sub(struct evhttp_request *req){
	struct evkeyvalq params;
	const char *uri = evhttp_request_get_uri(req);
	evhttp_parse_query(uri, &params);

	struct evbuffer *buf;
	
	int uid = -1;
	struct evkeyval *kv;
	for(kv = params.tqh_first; kv; kv = kv->next.tqe_next){
		if(strcmp(kv->key, "id") == 0){
			uid = atoi(kv->value);
		}
	}
	
	if(uid < 0 || uid >= MAX_CHANNELS){
		buf = evbuffer_new();
		evhttp_send_reply_start(req, HTTP_NOTFOUND, "Not Found");
		evbuffer_free(buf);
		return 0;
	}
	
	Channel *channel = &channels[uid];
	Subscriber *sub = sub_pool.alloc();
	sub->req = req;
	sub->ptr = this;
	//sub->last_recv = ...
	channel->add_subscriber(sub);
	log_debug("channel: %d, add sub, sub_count: %d", channel->id, channel->sub_count);

	buf = evbuffer_new();
	evhttp_send_reply_start(req, HTTP_OK, "OK");
	evhttp_add_header(req->output_headers, "Content-Type", "text/html; charset=utf-8");
	
	evbuffer_add_printf(buf, "{type: \"welcome\", id: \"%d\", content: \"hello world!\"}\n", uid);
	evhttp_send_reply_chunk(req, buf);
	evbuffer_free(buf);

	evhttp_connection_set_closecb(req->evcon, on_disconnect, sub);
	return 0;
}

int Server::pub(struct evhttp_request *req){
	struct evkeyvalq params;
	const char *uri = evhttp_request_get_uri(req);
	evhttp_parse_query(uri, &params);

	int uid = -1;
	const char *content = "";
	struct evkeyval *kv;
	for(kv = params.tqh_first; kv; kv = kv->next.tqe_next){
		if(strcmp(kv->key, "id") == 0){
			uid = atoi(kv->value);
		}else if(strcmp(kv->key, "content") == 0){
			content = kv->value;
		}
	}
	
	Channel *channel = NULL;
	if(uid < 0 || uid >= MAX_CHANNELS){
		channel = NULL;
	}else{
		channel = &channels[uid];
		log_debug("channel: %d, pub, sub_count: %d", channel->id, channel->sub_count);
	}
	if(!channel || !channel->subs){
		struct evbuffer *buf = evbuffer_new();
		evbuffer_add_printf(buf, "id: %d not connected\n", uid);
		evhttp_send_reply(req, 404, "Not Found", buf);
		evbuffer_free(buf);
		return 0;
	}

	struct evbuffer *buf = evbuffer_new();
	for(Subscriber *sub = channel->subs; sub; sub=sub->next){
		printf("pub: %d content: %s\n", uid, content);
		
		// push to subscriber
		evbuffer_add_printf(buf, "{type: \"data\", id: \"%d\", content: \"%s\"}\n", uid, content);
		evhttp_send_reply_chunk(sub->req, buf);
		
		// response to publisher
		evhttp_add_header(req->output_headers, "Content-Type", "text/html; charset=utf-8");
		evbuffer_add_printf(buf, "ok\n");
		evhttp_send_reply(req, 200, "OK", buf);
	}
	evbuffer_free(buf);

	return 0;
}
