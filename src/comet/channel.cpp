/*
Copyright (c) 2012-2014 The icomet Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "channel.h"
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include "util/log.h"
#include "util/list.h"
#include "server.h"
#include "subscriber.h"
#include "server_config.h"

Channel::Channel(){
	serv = NULL;
	idle = 0;
	seq_next = 1;
	presence_idle = 0;
}

Channel::~Channel(){
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

void Channel::close(){
	this->send("close", "");
	LinkedList<Subscriber *>::Iterator it = subs.iterator();
	while(Subscriber *sub = it.next()){
		sub->close();
	}
}

static std::string json_encode(const char *str){
	std::string ret;
	int len = strlen(str);
	for(int i=0; i<len; i++){
		char c = str[i];
		if(c == '"'){
			ret.append("\\\"", 2);
		}else if(c == '\\'){
			ret.append("\\\\", 2);
		}else if(c == '\r'){
			ret.append("\\r", 2);
		}else if(c == '\n'){
			ret.append("\\n", 2);
		}else{
			ret.push_back(c);
		}
	}
	return ret;
}

void Channel::send(const char *type, const char *content, bool encoded){
	Message msg;
	msg.seq = seq_next;
	msg.set_type_text(type);
	if(encoded){
		msg.content = content;
	}else{
		msg.content = json_encode(content);
	}

	LinkedList<Subscriber *>::Iterator it = subs.iterator();
	while(Subscriber *sub = it.next()){
		sub->send_chunk(msg.seq, type, msg.content.c_str());
	}

	if(msg.type == Message::DATA || msg.type == Message::BROADCAST){
		msg_list.push_back(msg);
		seq_next ++;
	}
	
	if(msg_list.size() >= ServerConfig::channel_buffer_size * 1.5){
		std::vector<Message>::iterator it;
		it = msg_list.end() - ServerConfig::channel_buffer_size;
		msg_list.assign(it, msg_list.end());
		log_trace("resize msg_list to %d, seq_next: %d", msg_list.size(), seq_next);
	}
}
