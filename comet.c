#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <event.h>
#include <evhttp.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CHANNELS	1000

struct Channel{
	int id;
	struct evhttp_request *req;
};

struct Channel channels[MAX_CHANNELS];


void init(){
	int i;
	for(i=0; i<MAX_CHANNELS; i++){
		channels[i].id = i;
		channels[i].req = NULL;
	}
}

// called when user disconnects
void cleanup(struct evhttp_connection *evcon, void *arg){
	struct Channel *sub = (struct Channel *)arg;
    printf("disconnected uid %d\n", sub->id);
    sub->req = NULL;
}

void sub_handler(struct evhttp_request *req, void *arg)
{
	struct evkeyvalq params;
	const char *uri = evhttp_request_get_uri(req);
	evhttp_parse_query(uri, &params);

	struct evbuffer *buf;
	
	int uid = -1;
	struct evkeyval *kv;
	for(kv = params.tqh_first; kv; kv = kv->next.tqe_next){
		if(strcmp(kv->key, "id") == 0){
			uid = atoi(kv->value);
		}
	}
	
	if(uid < 0 || uid >= MAX_CHANNELS){
		buf = evbuffer_new();
		evhttp_send_reply_start(req, HTTP_NOTFOUND, "Not Found");
		evbuffer_free(buf);
		return;
	}
	
	printf("sub: %d\n", uid);
	struct Channel *sub = &channels[uid];
	sub->req = req;

	buf = evbuffer_new();
	evhttp_send_reply_start(req, HTTP_OK, "OK");
	evhttp_add_header(req->output_headers, "Content-Type", "text/html; charset=utf-8");
	
	evbuffer_add_printf(buf, "{type: \"welcome\", id: \"%d\", content: \"hello world!\"}\n", uid);
	evhttp_send_reply_chunk(req, buf);
	evbuffer_free(buf);

	evhttp_connection_set_closecb(req->evcon, cleanup, &channels[uid]);
}

void pub_handler(struct evhttp_request *req, void *arg){
	struct evkeyvalq params;
	const char *uri = evhttp_request_get_uri(req);
	evhttp_parse_query(uri, &params);

	struct evbuffer *buf;
	
	int uid = -1;
	const char *content = "";
	struct evkeyval *kv;
	for(kv = params.tqh_first; kv; kv = kv->next.tqe_next){
		if(strcmp(kv->key, "id") == 0){
			uid = atoi(kv->value);
		}else if(strcmp(kv->key, "content") == 0){
			content = kv->value;
		}
	}
	
	struct Channel *sub = NULL;
	if(uid < 0 || uid >= MAX_CHANNELS){
		sub = NULL;
	}else{
		sub = &channels[uid];
	}
	if(sub && sub->req){
		printf("pub: %d content: %s\n", uid, content);
		
		// push to browser
		buf = evbuffer_new();
		evbuffer_add_printf(buf, "{type: \"data\", id: \"%d\", content: \"%s\"}\n", uid, content);
		evhttp_send_reply_chunk(sub->req, buf);
		evbuffer_free(buf);
		
		// response to publisher
		buf = evbuffer_new();
		evhttp_add_header(req->output_headers, "Content-Type", "text/html; charset=utf-8");
		evbuffer_add_printf(buf, "ok\n");
		evhttp_send_reply(req, 200, "OK", buf);
		evbuffer_free(buf);
	}else{
		buf = evbuffer_new();
		evbuffer_add_printf(buf, "id: %d not connected\n", uid);
		evhttp_send_reply(req, 404, "Not Found", buf);
		evbuffer_free(buf);
	}
}


int main(int argc, char **argv){
	signal(SIGPIPE, SIG_IGN);

	struct event_base *base;
	struct evhttp *http;
	struct evhttp_bound_socket *handle;
	unsigned short port = 8000;
	
	init();

	base = event_base_new();
	http = evhttp_new(base);
	evhttp_set_cb(http, "/sub", sub_handler, NULL);
	evhttp_set_cb(http, "/pub", pub_handler, NULL);
	//evhttp_set_gencb(http, request_handler, NULL);
	handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
	printf("server listen at 127.0.0.1:%d\n", port);
	event_base_dispatch(base);

	return 0;
}

