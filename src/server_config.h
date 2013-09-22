#ifndef ICOMET_SERVER_CONFIG_H
#define ICOMET_SERVER_CONFIG_H

#include "util/config.h"

class ServerConfig{
public:
	static int max_channels;
	static int max_messages_per_channel;
	static int max_subscribers_per_channel;
};

#endif
