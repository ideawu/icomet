#include "../config.h"
#include "channel.h"
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include "util/log.h"
#include "util/list.h"
#include "server.h"
#include "server_config.h"

Channel::Channel(){
	reset();
}

Channel::~Channel(){
}

void Channel::reset(){
	seq_next = 0;
	idle = -1;
	token.clear();
	msg_list.clear();
}

void Channel::add_subscriber(Subscriber *sub){
	sub->channel = this;
	subs.push_back(sub);
}

void Channel::del_subscriber(Subscriber *sub){
	sub->channel = NULL;
	subs.remove(sub);
}

void Channel::create_token(){
	// TODO: rand() is not safe?
	struct timeval tv;
	gettimeofday(&tv, NULL);
	token.resize(33);
	char *buf = (char *)token.data();
	int offset = 0;
	while(1){
		int r = rand() + tv.tv_usec;
		int len = snprintf(buf + offset, token.size() - offset, "%08x", r);
		if(len == -1){
			break;
		}
		offset += len;
		if(offset >= token.size() - 1){
			break;
		}
	}
	token.resize(32);
}

void Channel::send(const char *type, const char *content){
	struct evbuffer *buf = evbuffer_new();
	Subscriber *sub_next = NULL;
	for(Subscriber *sub = this->subs.head; sub; sub=sub_next){
		sub_next = sub->next;
		evbuffer_add_printf(buf,
			"%s({type: \"%s\", cid: \"%d\", seq: \"%d\", content: \"%s\"});\n",
			sub->callback.c_str(),
			type,
			this->id,
			this->seq_next,
			content);
		evhttp_send_reply_chunk(sub->req, buf);
		evhttp_send_reply_end(sub->req);
		//
		evhttp_connection_set_closecb(sub->req->evcon, NULL, NULL);
		sub->serv->sub_end(sub);
	}
	evbuffer_free(buf);

	if(strcmp(type, "data") == 0){
		msg_list.push_back(content);
		seq_next ++;
		if(msg_list.size() >= ServerConfig::channel_buffer_size * 1.5){
			std::vector<std::string>::iterator it;
			it = msg_list.end() - ServerConfig::channel_buffer_size;
			msg_list.assign(it, msg_list.end());
			log_trace("resize msg_list to %d, seq_next: %d", msg_list.size(), seq_next);
		}
	}
}
