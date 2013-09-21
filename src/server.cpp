#include <http-internal.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include "server.h"
#include "util/log.h"
#include "util/list.h"

Server::Server(){
	channel_slots.resize(MAX_CHANNELS);
	for(int i=0; i<channel_slots.size(); i++){
		Channel *channel = &channel_slots[i];
		channel->id = i;
	}
	list_reset(channels);
}

Server::~Server(){
}

void Server::add_channel(Channel *channel){
	assert(channel->subs.size == 0);
	list_add(channels, channel);
}

void Server::del_channel(Channel *channel){
	assert(channel->subs.size == 0);
	list_del(channels, channel);
}

static void on_connection_close(struct evhttp_connection *evcon, void *arg){
	log_debug("connection closed");
	Subscriber *sub = (Subscriber *)arg;
	Server *serv = sub->serv;
	serv->sub_end(sub);
}

int Server::sub_end(Subscriber *sub){
	Channel *channel = sub->channel;
	channel->del_subscriber(sub);
	sub_pool.free(sub);
	
	if(channel->subs.size == 0){
		this->del_channel(channel);
	}
	log_debug("channels: %d, ch: %d, del sub, subs: %d",
		channels.size, channel->id, channel->subs.size);
	return 0;
}

int Server::check_timeout(){
	//log_debug("<");
	struct evbuffer *buf = evbuffer_new();
	for(Channel *channel = channels.head; channel; channel=channel->next){
		if(!channel->subs.size){
			continue;
		}
		for(Subscriber *sub = channel->subs.head; sub; sub=sub->next){
			if(++sub->idle < SUB_MAX_IDLES){
				continue;
			}
			evbuffer_add_printf(buf,
				"%s({type: \"noop\", cid: \"%d\", seq: \"%d\"});\n",
				sub->callback.c_str(),
				channel->id,
				sub->noop_seq
				);
			evhttp_send_reply_chunk(sub->req, buf);
			evhttp_send_reply_end(sub->req);
			//
			evhttp_connection_set_closecb(sub->req->evcon, NULL, NULL);
			this->sub_end(sub);
		}
	}
	evbuffer_free(buf);
	//log_debug(">");
	return 0;
}

int Server::sub(struct evhttp_request *req){
	if(evhttp_request_get_command(req) != EVHTTP_REQ_GET){
		evhttp_send_reply(req, 405, "Invalid Method", NULL);
		return 0;
	}
	bufferevent_enable(evhttp_connection_get_bufferevent(req->evcon), EV_READ);
	
	struct evkeyvalq params;
	const char *uri = evhttp_request_get_uri(req);
	evhttp_parse_query(uri, &params);

	int cid = -1;
	int seq = 0;
	int noop = 0;
	const char *cb = DEFAULT_JSONP_CALLBACK;
	//const char *token = NULL;
	struct evkeyval *kv;
	for(kv = params.tqh_first; kv; kv = kv->next.tqe_next){
		if(strcmp(kv->key, "id") == 0){
			cid = atoi(kv->value);
		}else if(strcmp(kv->key, "seq") == 0){
			seq = atoi(kv->value);
		}else if(strcmp(kv->key, "noop") == 0){
			noop = atoi(kv->value);
		}else if(strcmp(kv->key, "cb") == 0){
			cb = kv->value;
		}
	}
	
	if(cid < 0 || cid >= channel_slots.size()){
		struct evbuffer *buf = evbuffer_new();
		evbuffer_add_printf(buf,
			"%s({type: \"404\", cid: \"%d\", seq: \"0\", content: \"Not Found\"});\n",
			cb,
			cid);
		evhttp_send_reply(req, HTTP_OK, "OK", buf);
		evbuffer_free(buf);
		return 0;
	}
	
	Channel *channel = &channel_slots[cid];
	if(channel->subs.size >= MAX_SUBS_PER_CHANNEL){
		struct evbuffer *buf = evbuffer_new();
		evbuffer_add_printf(buf,
			"%s({type: \"429\", cid: \"%d\", seq: \"0\", content: \"Too Many Requests\"});\n",
			cb,
			cid);
		evhttp_send_reply(req, HTTP_OK, "OK", buf);
		evbuffer_free(buf);
		return 0;
	}
	
	Subscriber *sub = sub_pool.alloc();
	sub->req = req;
	sub->serv = this;
	sub->idle = 0;
	sub->noop_seq = noop;
	sub->callback = cb;
	
	if(channel->subs.size == 0){
		this->add_channel(channel);
	}
	channel->add_subscriber(sub);
	log_debug("channels: %d, ch: %d, add sub, subs: %d",
		channels.size, channel->id, channel->subs.size);

	evhttp_add_header(req->output_headers, "Content-Type", "text/javascript; charset=utf-8");
	evhttp_send_reply_start(req, HTTP_OK, "OK");

	evhttp_connection_set_closecb(req->evcon, on_connection_close, sub);
	return 0;
}

int Server::ping(struct evhttp_request *req){
	struct evkeyvalq params;
	const char *uri = evhttp_request_get_uri(req);
	evhttp_parse_query(uri, &params);

	const char *cb = DEFAULT_JSONP_CALLBACK;
	struct evkeyval *kv;
	for(kv = params.tqh_first; kv; kv = kv->next.tqe_next){
		if(strcmp(kv->key, "cb") == 0){
			cb = kv->value;
		}
	}

	evhttp_add_header(req->output_headers, "Content-Type", "text/javascript; charset=utf-8");
	struct evbuffer *buf = evbuffer_new();
	evbuffer_add_printf(buf,
		"%s({type: \"ping\", sub_timeout: %d});\n",
		cb,
		SUB_IDLE_TIMEOUT);
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);
	return 0;
}

int Server::pub(struct evhttp_request *req){
	if(evhttp_request_get_command(req) != EVHTTP_REQ_GET){
		evhttp_send_reply(req, 405, "Invalid Method", NULL);
		return 0;
	}
	
	struct evkeyvalq params;
	const char *uri = evhttp_request_get_uri(req);
	evhttp_parse_query(uri, &params);

	int cid = -1;
	const char *content = "";
	struct evkeyval *kv;
	for(kv = params.tqh_first; kv; kv = kv->next.tqe_next){
		if(strcmp(kv->key, "id") == 0){
			cid = atoi(kv->value);
		}else if(strcmp(kv->key, "content") == 0){
			content = kv->value;
		}
	}
	
	Channel *channel = NULL;
	if(cid < 0 || cid >= MAX_CHANNELS){
		channel = NULL;
	}else{
		channel = &channel_slots[cid];
	}
	if(!channel || !channel->subs.size){
		struct evbuffer *buf = evbuffer_new();
		evbuffer_add_printf(buf, "id: %d not connected\n", cid);
		evhttp_send_reply(req, 404, "Not Found", buf);
		evbuffer_free(buf);
		return 0;
	}
	log_debug("ch: %d, subs: %d, pub content: %s", channel->id, channel->subs.size, content);

	// push to subscribers
	channel_send(channel, "data", content);
		
	// response to publisher
	struct evbuffer *buf = evbuffer_new();
	evhttp_add_header(req->output_headers, "Content-Type", "text/html; charset=utf-8");
	evbuffer_add_printf(buf, "ok %d\n", channel->seq_send);
	evhttp_send_reply(req, 200, "OK", buf);
	evbuffer_free(buf);

	return 0;
}

void Server::channel_send(Channel *channel, const char *type, const char *content){
	struct evbuffer *buf = evbuffer_new();
	for(Subscriber *sub = channel->subs.head; sub; sub=sub->next){
		evbuffer_add_printf(buf,
			"%s({type: \"%s\", cid: \"%d\", seq: \"%d\", content: \"%s\"});\n",
			sub->callback.c_str(),
			type,
			channel->id,
			channel->seq_send,
			content);
		evhttp_send_reply_chunk(sub->req, buf);
		evhttp_send_reply_end(sub->req);
		//
		evhttp_connection_set_closecb(sub->req->evcon, NULL, NULL);
		this->sub_end(sub);
	}
	evbuffer_free(buf);

	if(strcmp(type, "data") == 0){
		channel->seq_send ++;
	}
}

