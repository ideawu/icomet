#include "channel.h"
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include "util/log.h"
#include "util/list.h"

Channel::Channel(){
	this->seq_send = 0;
	list_reset(subs);
}

Channel::~Channel(){
}

void Channel::add_subscriber(Subscriber *sub){
	sub->channel = this;
	list_add(subs, sub);
}

void Channel::del_subscriber(Subscriber *sub){
	sub->channel = NULL;
	list_del(subs, sub);
}
