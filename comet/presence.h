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
};

#endif
