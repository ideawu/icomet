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
	idle = 0;
}

Subscriber::~Subscriber(){
}

static void connection_closecb(struct evhttp_connection *evcon, void *arg){
	Subscriber *sub = (Subscriber *)arg;
	log_debug("subscriber disconnected");
	sub->close();
}

static void add_http_header(int sub_type, struct evhttp_request *req){
	evhttp_add_header(req->output_headers, "Expires", "0");
	evhttp_add_header(req->output_headers, "Cache-Control", "no-cache");
	if(sub_type == Subscriber::SSE){
		evhttp_add_header(req->output_headers, "Content-Type", "text/event-stream; charset=utf-8");
		// 允许客户端跨域请求
		evhttp_add_header(req->output_headers, "Access-Control-Allow-Origin", "*");
	}else{
		evhttp_add_header(req->output_headers, "Content-Type", "application/json; charset=utf-8");
	}
}

void Subscriber::start(){
	log_debug("%s:%d sub %s, seq: %d, subs: %d",
		req->remote_host, req->remote_port,
		channel->name.c_str(), this->seq_next, channel->subs.size);

	// 需要监控 EV_READ 事件，否则无法得知客户端主动关闭连接。
	bufferevent_enable(req->evcon->bufev, EV_READ);
	// evhttp_send_reply_start 与 evhttp_send_reply_end 必须成对出现, 否则内存泄露。
	// 有两个地方需要调用 evhttp_send_reply_end(), 一是服务端主动关闭，二是客户端主动关闭
	// 客户端主动关闭会触发 connection_closecb
	evhttp_connection_set_closecb(req->evcon, connection_closecb, this);

	add_http_header(this->type, req);
	evhttp_add_header(req->output_headers, "Connection", "keep-alive");
	evhttp_send_reply_start(req, HTTP_OK, "OK");

	if(this->type == Subscriber::IFRAME){
		struct evbuffer *buf = evhttp_request_get_output_buffer(req);
		evbuffer_add_printf(buf, "%s\n", iframe_header.c_str());
		evhttp_send_reply_chunk(req, buf);
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

void Subscriber::close(){
	log_debug("%s:%d end %s, subs: %d,",
		this->req->remote_host, this->req->remote_port,
		channel->name.c_str(), channel->subs.size);
	if(req->evcon){
		// 记得清除 closecb
		evhttp_connection_set_closecb(req->evcon, NULL, NULL);
	}
	evhttp_send_reply_end(req);
	channel->serv->sub_end(this);
}

void Subscriber::send_old_msgs(){
	std::vector<Message>::iterator it = channel->msg_list.begin();
	while(1){
		if(it == channel->msg_list.end()){
			return;
		}
		const Message &msg = *it;
		if(msg.seq >= this->seq_next){
			log_debug("%s:%d send old msg [%d ~ %d]", req->remote_host, req->remote_port,
				msg.seq, this->channel->seq_next-1);
			break;
		}
		it ++;
	}

	if(this->type == POLL){
		Subscriber::send_start(this->type, this->req, this->callback.c_str(), true);
		for(int i=0; it != channel->msg_list.end(); i++, it++){
			const Message &msg = *it;
			int is_arr = (i != 0);
			Subscriber::send_msg(this->req, msg.get_type_text(),
				this->channel->name.c_str(), msg.seq, msg.content.c_str(), is_arr);
			// 兼容老的客户端, 因为老的客户端遇到 broadcast 时没有将自己的 seq+1
			if(msg.type == Message::BROADCAST){
				Subscriber::send_msg(this->req, "next_seq",
					this->channel->name.c_str(), msg.seq+1, "", true);
			}
		}
		Subscriber::send_end(this->type, this->req, this->callback.c_str(), true);
		this->close();
	}else{
		for(/**/; it != channel->msg_list.end(); it++){
			const Message &msg = *it;
			this->send_chunk(msg.seq, msg.get_type_text(), msg.content.c_str());
		}
	}
}

void Subscriber::noop(){
	this->send_chunk(this->seq_noop, "noop", "");
	this->seq_noop ++;
}

void Subscriber::sync_next_seq(){
	log_debug("%s:%d send sync_next_seq: %d", req->remote_host, req->remote_port, seq_next);
	this->send_chunk(seq_next, "next_seq", NULL);
}

void Subscriber::send_chunk(int seq, const char *type, const char *content){
	this->idle = 0;
	if(strcmp(type, "data") == 0 || strcmp(type, "broadcast") == 0){
		this->seq_next = seq + 1;
	}
	
	if(this->type == POLL && strcmp(type, "broadcast") == 0){
		Subscriber::send_start(this->type, this->req, this->callback.c_str(), true);
		Subscriber::send_msg(this->req, type, this->channel->name.c_str(), seq, content);
		// 兼容老的客户端, 因为老的客户端遇到 broadcast 时没有将自己的 seq+1
		Subscriber::send_msg(this->req, "next_seq", this->channel->name.c_str(), this->seq_next, "", true);
		Subscriber::send_end(this->type, this->req, this->callback.c_str(), true);
	}else{
		Subscriber::send_start(this->type, this->req, this->callback.c_str());
		Subscriber::send_msg(this->req, type, this->channel->name.c_str(), seq, content);
		Subscriber::send_end(this->type, this->req, this->callback.c_str());
		// 兼容老的客户端, 因为老的客户端遇到 broadcast 时没有将自己的 seq+1
		if(strcmp(type, "broadcast") == 0){
			this->sync_next_seq();
		}
	}
	if(this->type == POLL){
		this->close();
	}
}

void Subscriber::send_start(int sub_type, struct evhttp_request *req, const char *cb, bool is_arr){
	struct evbuffer *buf = evhttp_request_get_output_buffer(req);
	if(sub_type == POLL){
		if(cb != NULL && cb[0] != '\0'){
			evbuffer_add_printf(buf, "%s(", cb);
		}
		if(is_arr){
			evbuffer_add_printf(buf, "[");
		}
	}else if(sub_type == IFRAME){
		evbuffer_add_printf(buf, "%s", iframe_chunk_prefix.c_str());
	}else if(sub_type == SSE){
		evbuffer_add_printf(buf, "data: ");
	}
}

void Subscriber::send_end(int sub_type, struct evhttp_request *req, const char *cb, bool is_arr){
	struct evbuffer *buf = evhttp_request_get_output_buffer(req);
	if(sub_type == POLL){
		if(is_arr){
			evbuffer_add_printf(buf, "]");
		}
		if(cb != NULL && cb[0] != '\0'){
			evbuffer_add_printf(buf, ");");
		}
	}else if(sub_type == IFRAME){
		evbuffer_add_printf(buf, "%s", iframe_chunk_suffix.c_str());
	}else if(sub_type == SSE){
		evbuffer_add_printf(buf, "\n");
	}
	evbuffer_add_printf(buf, "\n");
	evhttp_send_reply_chunk(req, buf);
}

void Subscriber::send_msg(struct evhttp_request *req, const char *type, const std::string &cname, int seq,  const char *content, bool is_arr){
	struct evbuffer *buf = evhttp_request_get_output_buffer(req);
	if(content == NULL){
		content = "";
	}
	if(is_arr){
		evbuffer_add(buf, ",", 1);
	}
	evbuffer_add_printf(buf,
		"{\"type\":\"%s\",\"cname\":\"%s\",\"seq\":%d,\"content\":\"%s\"}",
		type, cname.c_str(), seq, content);
}

void Subscriber::send_error_reply(int sub_type, struct evhttp_request *req, const char *cb, const std::string &cname, const char *type, const char *content){
	add_http_header(sub_type, req);
	evhttp_send_reply_start(req, HTTP_OK, "OK");
	if(sub_type == Subscriber::IFRAME){
		struct evbuffer *buf = evhttp_request_get_output_buffer(req);
		evbuffer_add_printf(buf, "%s\n", iframe_header.c_str());
		evhttp_send_reply_chunk(req, buf);
	}

	Subscriber::send_start(sub_type, req, cb);
	Subscriber::send_msg(req, type, cname, 0, content);
	Subscriber::send_end(sub_type, req, cb);
	evhttp_send_reply_end(req);
}

