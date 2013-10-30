#ifndef ICOMET_PRESENCE_SUBSCRIBER_H
#define ICOMET_PRESENCE_SUBSCRIBER_H

#include <evhttp.h>
#include <event2/http.h>

class Server;

class PresenceSubscriber
{
public:
	PresenceSubscriber *prev;
	PresenceSubscriber *next;

	Server *serv;
	struct evhttp_request *req;
};

#endif
