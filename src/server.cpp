#include "../config.h"
#include <http-internal.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include "server.h"
#include "server_config.h"
#include "util/log.h"
#include "util/list.h"

Server::Server(){
	this->auth = AUTH_NONE;

	channel_slots.resize(ServerConfig::max_channels);
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
	log_trace("connection closed");
	Subscriber *sub = (Subscriber *)arg;
	Server *serv = sub->serv;
	serv->sub_end(sub);
}

int Server::check_timeout(){
	//log_debug("<");
	struct evbuffer *buf = evbuffer_new();
	for(Channel *channel = channels.head; channel; channel=channel->next){
		if(channel->subs.size == 0){
			if(--channel->idle < 0){
				log_trace("delete channel: %d", channel->id);
				this->del_channel(channel);
				channel->reset();
			}
			continue;
		}
		if(channel->idle < CHANNEL_MAX_IDLES){
			channel->idle = CHANNEL_MAX_IDLES;
		}

		for(Subscriber *sub = channel->subs.head; sub; sub=sub->next){
			if(++sub->idle <= SUB_MAX_IDLES){
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

int Server::sub_end(Subscriber *sub){
	Channel *channel = sub->channel;
	channel->del_subscriber(sub);
	sub_pool.free(sub);
	
	log_trace("channels: %d, ch: %d, del sub, subs: %d",
		channels.size, channel->id, channel->subs.size);
	return 0;
}

int Server::sub(struct evhttp_request *req){
	if(evhttp_request_get_command(req) != EVHTTP_REQ_GET){
		evhttp_send_reply(req, 405, "Method Not Allowed", NULL);
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
	const char *token = "";
	struct evkeyval *kv;
	for(kv = params.tqh_first; kv; kv = kv->next.tqe_next){
		if(strcmp(kv->key, "cid") == 0){
			cid = atoi(kv->value);
		}else if(strcmp(kv->key, "seq") == 0){
			seq = atoi(kv->value);
		}else if(strcmp(kv->key, "noop") == 0){
			noop = atoi(kv->value);
		}else if(strcmp(kv->key, "cb") == 0){
			cb = kv->value;
		}else if(strcmp(kv->key, "token") == 0){
			token = kv->value;
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
	if(this->auth == AUTH_TOKEN &&
		(channel->idle == -1 || channel->token.empty() || channel->token != token))
	{
		log_debug("%s:%d, Token Error, cid: %d, token: %s",
			req->remote_host,
			req->remote_port,
			cid,
			token
			);
		struct evbuffer *buf = evbuffer_new();
		evbuffer_add_printf(buf,
			"%s({type: \"401\", cid: \"%d\", seq: \"0\", content: \"Token Error\"});\n",
			cb,
			cid);
		evhttp_send_reply(req, HTTP_OK, "OK", buf);
		evbuffer_free(buf);
		return 0;
	}
	if(channel->subs.size >= ServerConfig::max_subscribers_per_channel){
		log_debug("%s:%d, Too Many Requests, cid: %d",
			req->remote_host,
			req->remote_port,
			cid
			);
		struct evbuffer *buf = evbuffer_new();
		evbuffer_add_printf(buf,
			"%s({type: \"429\", cid: \"%d\", seq: \"0\", content: \"Too Many Requests\"});\n",
			cb,
			cid);
		evhttp_send_reply(req, HTTP_OK, "OK", buf);
		evbuffer_free(buf);
		return 0;
	}
	if(channel->idle == -1){
		log_trace("new channel: %d", channel->id);
		this->add_channel(channel);
	}
	channel->idle = CHANNEL_MAX_IDLES;

	evhttp_add_header(req->output_headers, "Content-Type", "text/javascript; charset=utf-8");
	evhttp_add_header(req->output_headers, "Cache-Control", "no-cache");
	evhttp_add_header(req->output_headers, "Expires", "0");
	
	if(!channel->msg_list.empty() && channel->seq_next != seq){
		std::vector<std::string>::iterator it = channel->msg_list.end();
		int msg_seq_min = channel->seq_next - channel->msg_list.size();
		if(Channel::SEQ_GT(seq, channel->seq_next) || Channel::SEQ_GT(msg_seq_min, seq)){
			seq = msg_seq_min;
		}
		log_debug("send old msg: [%d, %d]", seq, channel->seq_next - 1);
		it -= (channel->seq_next - seq);

		struct evbuffer *buf = evbuffer_new();
		evbuffer_add_printf(buf, "%s([", cb);
		for(/**/; it != channel->msg_list.end(); it++, seq++){
			std::string &msg = *it;
			evbuffer_add_printf(buf,
				"{type: \"data\", cid: \"%d\", seq: \"%d\", content: \"%s\"}",
				cid,
				seq,
				msg.c_str());
			if(seq != channel->seq_next - 1){
				evbuffer_add(buf, ",", 1);
			}
		}
		evbuffer_add_printf(buf, "]);\n");
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
	
	channel->add_subscriber(sub);
	log_trace("channels: %d, ch: %d, add sub, subs: %d",
		channels.size, channel->id, channel->subs.size);

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
	evhttp_add_header(req->output_headers, "Cache-Control", "no-cache");
	evhttp_add_header(req->output_headers, "Expires", "0");
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
		if(strcmp(kv->key, "cid") == 0){
			cid = atoi(kv->value);
		}else if(strcmp(kv->key, "content") == 0){
			content = kv->value;
		}
	}
	
	Channel *channel = NULL;
	if(cid < 0 || cid >= ServerConfig::max_channels){
		channel = NULL;
	}else{
		channel = &channel_slots[cid];
	}
	if(!channel || channel->idle == -1){
		log_trace("channel %d offline, pub content: %s", cid, content);
		struct evbuffer *buf = evbuffer_new();
		evbuffer_add_printf(buf, "channel %d not connected\n", cid);
		evhttp_send_reply(req, 404, "Not Found", buf);
		evbuffer_free(buf);
		return 0;
	}
	log_debug("ch: %d, subs: %d, pub content: %s", channel->id, channel->subs.size, content);
		
	// response to publisher
	evhttp_add_header(req->output_headers, "Content-Type", "text/html; charset=utf-8");
	struct evbuffer *buf = evbuffer_new();
	evbuffer_add_printf(buf, "ok %d\n", channel->seq_next);
	evhttp_send_reply(req, 200, "OK", buf);
	evbuffer_free(buf);

	// push to subscribers
	channel->send("data", content);

	return 0;
}

int Server::sign(struct evhttp_request *req){
	if(evhttp_request_get_command(req) != EVHTTP_REQ_GET){
		evhttp_send_reply(req, 405, "Invalid Method", NULL);
		return 0;
	}
	
	struct evkeyvalq params;
	const char *uri = evhttp_request_get_uri(req);
	evhttp_parse_query(uri, &params);

	int cid = -1;
	int expires = 0;
	const char *cb = NULL;
	struct evkeyval *kv;
	for(kv = params.tqh_first; kv; kv = kv->next.tqe_next){
		if(strcmp(kv->key, "cid") == 0){
			cid = atoi(kv->value);
		}else if(strcmp(kv->key, "cb") == 0){
			cb = kv->value;
		}else if(strcmp(kv->key, "expires") == 0){
			expires = atoi(kv->value);
		}
	}
	if(expires <= 0){
		expires = CHANNEL_IDLE_TIMEOUT;
	}
	
	Channel *channel = NULL;
	if(cid < 0 || cid >= ServerConfig::max_channels){
		channel = NULL;
	}else{
		channel = &channel_slots[cid];
	}
	
	if(!channel){
		struct evbuffer *buf = evbuffer_new();
		evbuffer_add_printf(buf, "Invalid channel: %d\n", cid);
		evhttp_send_reply(req, 404, "Not Found", buf);
		evbuffer_free(buf);
		return 0;
	}

	if(channel->token.empty()){
		channel->create_token();
	}
	if(channel->idle == -1){
		log_debug("sign: %d, token: %s, expires: %d",
			channel->id, channel->token.c_str(), expires);
		this->add_channel(channel);
	}else{
		log_debug("re-sign: %d, token: %s, expires: %d",
			channel->id, channel->token.c_str(), expires);
	}
	channel->idle = expires/CHANNEL_CHECK_INTERVAL;

	evhttp_add_header(req->output_headers, "Content-Type", "text/html; charset=utf-8");
	struct evbuffer *buf = evbuffer_new();
	if(cb){
		evbuffer_add_printf(buf, "%s(", cb);
	}
	evbuffer_add_printf(buf,
		"{type: \"sign\", cid: %d, seq: %d, token: \"%s\", expires: %d, sub_timeout: %d}\n",
		channel->id,
		channel->msg_seq_min(),
		channel->token.c_str(),
		expires,
		SUB_IDLE_TIMEOUT);
	if(cb){
		evbuffer_add_printf(buf, ");\n");
	}
	evhttp_send_reply(req, 200, "OK", buf);
	evbuffer_free(buf);

	return 0;
}
