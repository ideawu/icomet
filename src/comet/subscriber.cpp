/*
Copyright (c) 2012-2014 The icomet Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "subscriber.h"
#include "channel.h"
#include "server.h"
#include "util/log.h"
#include "server_config.h"
#include <http-internal.h>

static std::string iframe_header = "<html><head><meta http-equiv='Content-Type' content='text/html; charset=utf-8'><meta http-equiv='Cache-Control' content='no-store'><meta http-equiv='Cache-Control' content='no-cache'><meta http-equiv='Pragma' content='no-cache'><meta http-equiv=' Expires' content='Thu, 1 Jan 1970 00:00:00 GMT'><script type='text/javascript'>window.onError = null;try{document.domain = window.location.hostname.split('.').slice(-2).join('.');}catch(e){};</script></head><body>";
static std::string iframe_chunk_prefix = "<script>parent.icomet_cb(";
static std::string iframe_chunk_suffix = ");</script>";

Subscriber::Subscriber(){
	req = NULL;
}

Subscriber::~Subscriber(){
}

static void on_sub_disconnect(struct evhttp_connection *evcon, void *arg){
	log_debug("subscriber disconnected");
	Subscriber *sub = (Subscriber *)arg;
	sub->close();
}

void Subscriber::start(){
	bufferevent_enable(req->evcon->bufev, EV_READ);
	evhttp_connection_set_closecb(req->evcon, on_sub_disconnect, this);
	evhttp_add_header(req->output_headers, "Connection", "keep-alive");
	//evhttp_add_header(req->output_headers, "Cache-Control", "no-cache");
	//evhttp_add_header(req->output_headers, "Expires", "0");
	evhttp_add_header(req->output_headers, "Content-Type", "text/html; charset=utf-8");
	evhttp_send_reply_start(req, HTTP_OK, "OK");

	if(this->type == POLL){
		//
	}else if(this->type == IFRAME){
		struct evbuffer *buf = evhttp_request_get_output_buffer(this->req);
		evbuffer_add_printf(buf, "%s\n", iframe_header.c_str());
		evhttp_send_reply_chunk(this->req, buf);
	}
	
	if(this->seq_next == 0 || Channel::SEQ_GT(this->seq_next, channel->seq_next)){
		this->seq_next = channel->seq_next;
		if(this->seq_next != 0){
			this->sync_next_seq();
		}
	}else{
		// send buffered messages
		if(!channel->msg_list.empty() && channel->seq_next != this->seq_next){
			this->send_old_msgs();
		}
	}
}

void Subscriber::send_old_msgs(){
	std::vector<Message>::iterator it = channel->msg_list.begin();
	while(1){
		if(it == channel->msg_list.end()){
			return;
		}
		const Message &msg = *it;
		if(msg.seq >= this->seq_next){
			log_debug("send old msg [%d ~ %d]", msg.seq, this->channel->seq_next-1);
			break;
		}
		it ++;
	}

	if(this->type == POLL){
		this->poll_send_start();
		int i = 0;
		for(/**/; it != channel->msg_list.end(); it++){
			const Message &msg = *it;
			int is_arr = (i != 0);
			i ++;
			this->poll_send(msg.seq, msg.get_type_text(), msg.content.c_str(), is_arr);
		}
		this->poll_send_end();
	}else{
		for(/**/; it != channel->msg_list.end(); it++){
			const Message &msg = *it;
			this->send_chunk(msg.seq, msg.get_type_text(), msg.content.c_str());
		}
	}
}

void Subscriber::close(){
	if(req->evcon){
		evhttp_connection_set_closecb(req->evcon, NULL, NULL);
	}
	evhttp_send_reply_end(req);
	channel->serv->sub_end(this);
}

void Subscriber::noop(){
	this->send_chunk(this->seq_noop, "noop", "");
}

void Subscriber::sync_next_seq(){
	log_debug("%s:%d sync_next_seq: %d", req->remote_host, req->remote_port, seq_next);
	this->send_chunk(seq_next, "next_seq", NULL);
}

void Subscriber::poll_send_start(bool array){
	struct evbuffer *buf = evhttp_request_get_output_buffer(this->req);
	if(!this->callback.empty()){
		evbuffer_add_printf(buf, "%s(", this->callback.c_str());
	}
	if(array){
		evbuffer_add_printf(buf, "[");
	}
}

void Subscriber::poll_send_end(bool array){
	struct evbuffer *buf = evhttp_request_get_output_buffer(this->req);
	if(array){
		evbuffer_add_printf(buf, "]");
	}
	if(!this->callback.empty()){
		evbuffer_add_printf(buf, ");");
	}
	evbuffer_add_printf(buf, "\n");
	evhttp_send_reply_chunk(this->req, buf);
	this->close();
}

void Subscriber::poll_send(int seq, const char *type, const char *content, bool array){
	this->idle = 0;
	if(strcmp(type, "data") == 0 || strcmp(type, "broadcast") == 0){
		this->seq_next = seq + 1;
	}
	if(content == NULL){
		content = "";
	}

	struct evbuffer *buf = evhttp_request_get_output_buffer(this->req);
	if(array){
		evbuffer_add(buf, ",", 1);
	}
	evbuffer_add_printf(buf,
		"{\"type\":\"%s\",\"cname\":\"%s\",\"seq\":%d,\"content\":\"%s\"}",
		type,
		this->channel->name.c_str(),
		seq,
		content);
	
	// 兼容老的客户端, 因为老的客户端遇到 broadcast 时没有将自己的 seq+1
	if(strcmp(type, "broadcast") == 0){
		this->poll_send(this->seq_next, "next_seq", "", true);
	}
}

void Subscriber::send_chunk(int seq, const char *type, const char *content){
	if(this->type == POLL){
		bool is_arr = strcmp(type, "broadcast") == 0;
		this->poll_send_start(is_arr);
		this->poll_send(seq, type, content, false);
		this->poll_send_end(is_arr);
		return;
	}

	this->idle = 0;
	if(strcmp(type, "data") == 0 || strcmp(type, "broadcast") == 0){
		this->seq_next = seq + 1;
	}
	if(content == NULL){
		content = "";
	}

	struct evbuffer *buf = evhttp_request_get_output_buffer(this->req);
	if(this->type == IFRAME){
		evbuffer_add_printf(buf, "%s", iframe_chunk_prefix.c_str());
	}
	evbuffer_add_printf(buf,
		"{\"type\":\"%s\",\"cname\":\"%s\",\"seq\":%d,\"content\":\"%s\"}",
		type, this->channel->name.c_str(), seq, content);
	if(this->type == IFRAME){
		evbuffer_add_printf(buf, "%s", iframe_chunk_suffix.c_str());
	}
	evbuffer_add_printf(buf, "\n");
	evhttp_send_reply_chunk(this->req, buf);

	if(strcmp(type, "broadcast") == 0){
		this->sync_next_seq();
	}
}

void Subscriber::send_error_reply(int sub_type, struct evhttp_request *req, const char *cb, const std::string &cname, const char *type, const char *content){
	struct evbuffer *buf = evhttp_request_get_output_buffer(req);
	
	if(sub_type == POLL){
		evbuffer_add_printf(buf, "%s(", cb);
	}else if(sub_type == IFRAME){
		evbuffer_add_printf(buf, "%s", iframe_chunk_prefix.c_str());
	}
	
	evbuffer_add_printf(buf,
		"{\"type\":\"%s\",\"cname\":\"%s\",\"seq\":%d,\"content\":\"%s\"}",
		type, cname.c_str(), 0, content);

	if(sub_type == POLL){
		evbuffer_add_printf(buf, ");");
	}else if(sub_type == IFRAME){
		evbuffer_add_printf(buf, "%s", iframe_chunk_suffix.c_str());
	}

	evbuffer_add_printf(buf, "\n");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
}

