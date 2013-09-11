#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <evhttp.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/bufferevent.h>
#include "util/log.h"

void chunk_cb(struct evhttp_request *req, void *arg){
	char buf[1024];
	int s = evbuffer_remove(req->input_buffer, &buf, sizeof(buf) - 1);
	buf[s] = '\0';
	printf("< %s", buf);
}

void http_request_done(struct evhttp_request *req, void *arg){
	log_debug("abc");
}

int main(int argc, char **argv){
	if(argc < 3){
		printf("Usage: %s start_id end_id\n", argv[0]);
		exit(0);
	}
	int start_id = atoi(argv[1]);
	int end_id = atoi(argv[2]);
	
	signal(SIGPIPE, SIG_IGN);

	struct event_base *base = event_base_new();
	if(!base){
		fprintf(stderr, "event_base_new() error!\n");
		exit(0);
	}

	struct evhttp_connection *conn;
	struct evhttp_request *req;
	const char *host = "127.0.0.1";
	int port = 8000;

	for(int i=start_id; i<=end_id; i++){
		log_debug("sub: %d", i);
		
		conn = evhttp_connection_base_new(base, NULL, host, port);
		req = evhttp_request_new(http_request_done, NULL);
		evhttp_request_set_chunked_cb(req, chunk_cb);

		char buf[128];
		snprintf(buf, sizeof(buf), "/sub?id=%d", i);
		evhttp_make_request(conn, req, EVHTTP_REQ_GET, buf);
	    evhttp_connection_set_timeout(req->evcon, 864000);
	    event_base_loop(base, EVLOOP_NONBLOCK);
		
		usleep(1 * 1000);
	}

	event_base_dispatch(base);
	
	log_debug("quit");
	return 0;
}

