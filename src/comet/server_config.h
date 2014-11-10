/*
Copyright (c) 2012-2014 The icomet Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef ICOMET_SERVER_CONFIG_H
#define ICOMET_SERVER_CONFIG_H

#include <string>
#include "util/config.h"

// initialized in icomet-server.cpp
class ServerConfig{
public:
	static int max_channels;
	static int max_subscribers_per_channel;
	static int polling_timeout;
	static int polling_idles; // max idle count to reconnect
	static int channel_buffer_size;
	static int channel_timeout;
	// rename max_channel_idles
	static int channel_idles; // max idle count to offline
	
	/*
	static std::string iframe_header;
	static std::string iframe_chunk_prefix;
	static std::string iframe_chunk_suffix;
	*/
	
	//int load(Config *conf);
};

#endif
