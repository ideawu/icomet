#include "channel.h"
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include "util/log.h"

Channel::Channel(){
	this->subs = NULL;
	this->sub_count = 0;
	this->seq_send = 0;
}

Channel::~Channel(){
}

void Channel::add_subscriber(Subscriber *sub){
	sub_count ++;
	if(this->subs){
		this->subs->prev = sub;
	}
	// add to front
	sub->prev = NULL;
	sub->next = this->subs;
	sub->channel = this;
	this->subs = sub;
}

void Channel::del_subscriber(Subscriber *sub){
	sub_count --;
	if(sub->prev){
		sub->prev->next = sub->next;
	}
	if(sub->next){
		sub->next->prev = sub->prev;
	}
	if(this->subs == sub){
		this->subs = sub->next;
	}
}

void Channel::send(const char *type, const char *content){
	struct evbuffer *buf = evbuffer_new();
	for(Subscriber *sub = this->subs; sub; sub=sub->next){
		evbuffer_add_printf(buf,
			"%s({type: \"%s\", id: \"%d\", seq: \"%d\" content: \"%s\"});\n",
			sub->cb.c_str(),
			type,
			this->id,
			this->seq_send,
			content);
		evhttp_send_reply_chunk(sub->req, buf);
	}
	evbuffer_free(buf);

	if(strcmp(type, "data") == 0){
		seq_send ++;
	}
}
