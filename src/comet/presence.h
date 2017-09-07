/*
Copyright (c) 2012-2014 The icomet Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef ICOMET_PRESENCE_H
#define ICOMET_PRESENCE_H

#include <evhttp.h>
#include <event2/http.h>

class Server;

enum PresenceType{
	PresenceOffline = 0,
	PresenceOnline  = 1
};

class PresenceSubscriber
{
public:
	PresenceSubscriber *prev;
	PresenceSubscriber *next;

	Server *serv;
	struct evhttp_request *req;
	
	void start();
	void close();
};

#endif
