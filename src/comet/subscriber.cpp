#include "subscriber.h"
#include "channel.h"
#include "server.h"
#include "util/log.h"
#include "server_config.h"

static void on_sub_disconnect(struct evhttp_connection *evcon, void *arg){
	log_debug("subscriber disconnected");
	Subscriber *sub = (Subscriber *)arg;
	Server *serv = sub->serv;
	serv->sub_end(sub);
}

void Subscriber::start(){
	//evhttp_add_header(req->output_headers, "Content-Type", "text/javascript; charset=utf-8");
	evhttp_add_header(req->output_headers, "Connection", "keep-alive");
	//evhttp_add_header(req->output_headers, "Cache-Control", "no-cache");
	//evhttp_add_header(req->output_headers, "Expires", "0");
	
	evhttp_send_reply_start(req, HTTP_OK, "OK");
	evhttp_connection_set_closecb(req->evcon, on_sub_disconnect, this);

	if(this->type == POLL){
		//
	}else if(this->type == STREAM){
		struct evbuffer *buf = evbuffer_new();
		evbuffer_add_printf(buf, "%s\n", ServerConfig::stream_header_template.c_str());
		evhttp_send_reply_chunk(this->req, buf);
		evbuffer_free(buf);
	}
	
	// send buffered messages
	if(!channel->msg_list.empty() && channel->seq_next != seq){
		this->send_old_msgs();
	}
}

void Subscriber::send_old_msgs(){
	std::vector<std::string>::iterator it = channel->msg_list.end();
	int msg_seq_min = channel->seq_next - channel->msg_list.size();
	if(Channel::SEQ_GT(seq, channel->seq_next) || Channel::SEQ_LT(seq, msg_seq_min)){
		seq = msg_seq_min;
	}
	log_debug("send old msg: [%d, %d]", seq, channel->seq_next - 1);
	it -= (channel->seq_next - seq);

	struct evbuffer *buf = evbuffer_new();
	if(this->type == POLL){
		evbuffer_add_printf(buf, "%s([", this->callback.c_str());
		for(/**/; it != channel->msg_list.end(); it++, seq++){
			std::string &msg = *it;
			evbuffer_add_printf(buf,
				"{type: \"data\", cname: \"%s\", seq: \"%d\", content: \"%s\"}",
				this->channel->name.c_str(),
				seq,
				msg.c_str());
			if(seq != channel->seq_next - 1){
				evbuffer_add(buf, ",", 1);
			}
		}
		evbuffer_add_printf(buf, "]);\n");
		evhttp_send_reply_chunk(this->req, buf);
		this->close();
	}else if(this->type == STREAM){
		for(/**/; it != channel->msg_list.end(); it++, seq++){
			std::string &msg = *it;
			if(!ServerConfig::stream_chunk_prefix_template.empty()){
				evbuffer_add_printf(buf, "%s", ServerConfig::stream_chunk_prefix_template.c_str());
			}
			evbuffer_add_printf(buf,
				"{type: \"data\", cname: \"%s\", seq: \"%d\", content: \"%s\"}",
				this->channel->name.c_str(),
				seq,
				msg.c_str());
			if(!ServerConfig::stream_chunk_suffix_template.empty()){
				evbuffer_add_printf(buf, "%s", ServerConfig::stream_chunk_suffix_template.c_str());
			}
			evbuffer_add_printf(buf, "\n");
			evhttp_send_reply_chunk(this->req, buf);
		}
	}
	evbuffer_free(buf);
}

void Subscriber::close(){
	evhttp_send_reply_end(this->req);
	evhttp_connection_set_closecb(this->req->evcon, NULL, NULL);
	this->serv->sub_end(this);
}

void Subscriber::send_chunk(const char *type, const char *content){
	struct evbuffer *buf = evbuffer_new();
	if(this->type == POLL){
		evbuffer_add_printf(buf,
			"%s({type: \"%s\", cname: \"%s\", seq: \"%d\", content: \"%s\"});\n",
			this->callback.c_str(),
			type,
			this->channel->name.c_str(),
			this->channel->seq_next,
			content);
		evhttp_send_reply_chunk(this->req, buf);
		this->close();
	}else if(this->type == STREAM){
		if(!ServerConfig::stream_chunk_prefix_template.empty()){
			evbuffer_add_printf(buf, "%s", ServerConfig::stream_chunk_prefix_template.c_str());
		}
		evbuffer_add_printf(buf,
			"{type: \"%s\", cname: \"%s\", seq: \"%d\", content: \"%s\"}",
			type,
			this->channel->name.c_str(),
			this->channel->seq_next,
			content);
		if(!ServerConfig::stream_chunk_suffix_template.empty()){
			evbuffer_add_printf(buf, "%s", ServerConfig::stream_chunk_suffix_template.c_str());
		}
		evbuffer_add_printf(buf, "\n");
		evhttp_send_reply_chunk(this->req, buf);
		this->idle = 0;
	}
	evbuffer_free(buf);
}

void Subscriber::noop(){
	if(this->type == POLL){
		struct evbuffer *buf = evbuffer_new();
		evbuffer_add_printf(buf,
			"%s({type: \"noop\", cname: \"%s\", seq: \"%d\", content: \"\"});\n",
			this->callback.c_str(),
			this->channel->name.c_str(),
			noop_seq);
		evhttp_send_reply_chunk(this->req, buf);
		this->close();
		evbuffer_free(buf);
	}else{
		this->send_chunk("noop", "");
	}
}

