/*
Copyright (c) 2012-2017 The icomet Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef ICOMET_HTTP_QUERY_H
#define ICOMET_HTTP_QUERY_H

#include <evhttp.h>
#include <event2/http.h>

class HttpQuery{
private:
	struct evkeyvalq _get;
	struct evkeyvalq _post;
	bool _has_post;
public:
	HttpQuery(struct evhttp_request *req){
		_has_post = false;
		if(evhttp_request_get_command(req) == EVHTTP_REQ_POST){
			evbuffer *body_evb = evhttp_request_get_input_buffer(req);
			size_t len = evbuffer_get_length(body_evb);
			if(len > 0){
				_has_post = true;
				char *data = (char *)malloc(len + 1);
				evbuffer_copyout(body_evb, data, len);
				data[len] = '\0';
				evhttp_parse_query_str(data, &_post);
				free(data);
			}
		}
		evhttp_parse_query(evhttp_request_get_uri(req), &_get);
	}
	~HttpQuery(){
		evhttp_clear_headers(&_get);
		if(_has_post){
			evhttp_clear_headers(&_post);
		}
	}
	int get_int(const char *name, int def){
		if(_has_post){
			const char *val = evhttp_find_header(&_post, name);
			if(val){
				return atoi(val);
			}
		}
		const char *val = evhttp_find_header(&_get, name);
		return val? atoi(val) : def;
	}
	const char* get_str(const char *name, const char *def){
		if(_has_post){
			const char *val = evhttp_find_header(&_post, name);
			if(val){
				return val;
			}
		}
		const char *val = evhttp_find_header(&_get, name);
		return val? val : def;
	}
};

#endif
