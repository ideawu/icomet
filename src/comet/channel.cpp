#include "../build.h"
#include "channel.h"
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include "util/log.h"
#include "util/list.h"
#include "server.h"
#include "subscriber.h"
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

void Channel::clear(){
	msg_list.clear();
}

void Channel::send(const char *type, const char *content){
	LinkedList<Subscriber *>::Iterator it = subs.iterator();
	while(Subscriber *sub = it.next()){
		sub->send_chunk(type, content);
	}

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

void Channel::send_old_msgs(struct evhttp_request *req, int next_seq, const char *cb){
	std::vector<std::string>::iterator it = this->msg_list.end();
	int msg_seq_min = this->seq_next - this->msg_list.size();
	if(Channel::SEQ_GT(next_seq, this->seq_next) || Channel::SEQ_LT(next_seq, msg_seq_min)){
		next_seq = msg_seq_min;
	}
	log_debug("send old msg: [%d, %d]", next_seq, this->seq_next - 1);
	it -= (this->seq_next - next_seq);

	struct evbuffer *buf = evbuffer_new();
	evbuffer_add_printf(buf, "%s([", cb);
	for(/**/; it != this->msg_list.end(); it++, next_seq++){
		std::string &msg = *it;
		evbuffer_add_printf(buf,
			"{type: \"data\", cname: \"%s\", seq: \"%d\", content: \"%s\"}",
			this->name.c_str(),
			next_seq,
			msg.c_str());
		if(next_seq != this->seq_next - 1){
			evbuffer_add(buf, ",", 1);
		}
	}
	evbuffer_add_printf(buf, "]);\n");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);
}

void Channel::error_token_error(struct evhttp_request *req, const char *cb, const char *token){
	log_debug("%s:%d, Token Error, cname: %s, token: %s",
		req->remote_host,
		req->remote_port,
		this->name.c_str(),
		token
		);
	struct evbuffer *buf = evbuffer_new();
	evbuffer_add_printf(buf,
		"%s({type: \"401\", cname: \"%s\", seq: \"0\", content: \"Token Error\"});\n",
		cb,
		this->name.c_str()
		);
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);
}

void Channel::error_too_many_subscribers(struct evhttp_request *req, const char *cb){
	log_debug("%s:%d, Too many subscribers, cname: %s",
		req->remote_host,
		req->remote_port,
		this->name.c_str()
		);
	struct evbuffer *buf = evbuffer_new();
	evbuffer_add_printf(buf,
		"%s({type: \"429\", cname: \"%s\", seq: \"0\", content: \"Too many subscribers\"});\n",
		cb,
		this->name.c_str()
		);
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);
}
