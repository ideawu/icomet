#include <http-internal.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include "server.h"
#include "util/log.h"

Server::Server(){
	channels.resize(MAX_CHANNELS);
	for(int i=0; i<channels.size(); i++){
		Channel *channel = &channels[i];
		channel->id = i;
	}
}

Server::~Server(){
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
	log_debug("channel: %d, del sub, sub_count: %d", channel->id, channel->sub_count);
	return 0;
}

int Server::check_timeout(){
	//log_debug("<");
	struct evbuffer *buf = evbuffer_new();
	for(int i=0; i<channels.size(); i++){
		Channel *channel = &channels[i];
		if(!channel->subs){
			continue;
		}

		for(Subscriber *sub = channel->subs; sub; sub=sub->next){
			if(++sub->idle < SUB_MAX_IDLES){
				continue;
			}
			evbuffer_add_printf(buf, "%s();\n", sub->cb.c_str());
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
	
	struct evkeyvalq params;
	const char *uri = evhttp_request_get_uri(req);
	evhttp_parse_query(uri, &params);

	struct bufferevent *bev = evhttp_connection_get_bufferevent(req->evcon);
	bufferevent_enable(bev, EV_READ);

	int cid = -1;
	const char *cb = NULL;
	//const char *token = NULL;
	struct evkeyval *kv;
	for(kv = params.tqh_first; kv; kv = kv->next.tqe_next){
		if(strcmp(kv->key, "id") == 0){
			cid = atoi(kv->value);
		}else if(strcmp(kv->key, "cb") == 0){
			cb = kv->value;
		}
	}
	
	if(cid < 0 || cid >= channels.size()){
		evhttp_send_reply(req, 404, "Not Found", NULL);
		return 0;
	}
	
	Channel *channel = &channels[cid];
	if(channel->sub_count >= MAX_SUBS_PER_CHANNEL){
		evhttp_send_reply(req, 429, "Too Many Requests", NULL);
		return 0;
	}
	
	Subscriber *sub = sub_pool.alloc();
	sub->req = req;
	sub->serv = this;
	sub->idle = 0;
	sub->cb = cb? cb : DEFAULT_JSONP_CALLBACK;
	//evutil_gettimeofday(&sub->time, NULL);
	//sub->last_recv = ...
	channel->add_subscriber(sub);
	log_debug("channel: %d, add sub, sub_count: %d", channel->id, channel->sub_count);

	evhttp_add_header(req->output_headers, "Content-Type", "text/javascript; charset=utf-8");
	evhttp_send_reply_start(req, HTTP_OK, "OK");

	evhttp_connection_set_closecb(req->evcon, on_connection_close, sub);

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
		channel = &channels[cid];
	}
	if(!channel || !channel->subs){
		struct evbuffer *buf = evbuffer_new();
		evbuffer_add_printf(buf, "id: %d not connected\n", cid);
		evhttp_send_reply(req, 404, "Not Found", buf);
		evbuffer_free(buf);
		return 0;
	}
	log_debug("pub: %d, sub_count: %d, content: %s", channel->id, channel->sub_count, content);

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
	for(Subscriber *sub = channel->subs; sub; sub=sub->next){
		evbuffer_add_printf(buf,
			"%s({type: \"%s\", cid: \"%d\", seq: \"%d\", content: \"%s\"});\n",
			sub->cb.c_str(),
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

