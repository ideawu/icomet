/*
Copyright (c) 2012-2017 The icomet Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "presence.h"
#include "server.h"
#include "util/log.h"
#include "server_config.h"
#include <http-internal.h>

static void connection_closecb(struct evhttp_connection *evcon, void *arg){
	log_info("presence subscriber disconnected");
	PresenceSubscriber *psub = (PresenceSubscriber *)arg;
	psub->close();
}

void PresenceSubscriber::start(){
	log_info("%s:%d psub, psubs: %d", req->remote_host, req->remote_port, serv->psubs.size);
	bufferevent_enable(req->evcon->bufev, EV_READ);
	evhttp_connection_set_closecb(req->evcon, connection_closecb, this);

	evhttp_send_reply_start(req, HTTP_OK, "OK");
}

void PresenceSubscriber::close(){
	log_info("%s:%d psub_end", req->remote_host, req->remote_port);
	if(req->evcon){
		evhttp_connection_set_closecb(req->evcon, NULL, NULL);
	}
	evhttp_send_reply_end(req);
	serv->psub_end(this);
}
