#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <evhttp.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/bufferevent.h>
#include "util/log.h"

#define MAX_BIND_PORTS 1

void chunk_cb(struct evhttp_request *req, void *arg){
	static int num = 0;
	if(++num % 1000 == 1){
		char buf[1024];
		int s = evbuffer_remove(req->input_buffer, &buf, sizeof(buf) - 1);
		buf[s] = '\0';
		printf("< %s", buf);
	}
}

void http_request_done(struct evhttp_request *req, void *arg){
	static int num = 0;
	if(++num % 1000 == 1){
		log_debug("request done %d", num);
	}
}

int main(int argc, char **argv){
	if(argc < 3){
		printf("Usage: %s ip port\n", argv[0]);
		exit(0);
	}
	const char *host = argv[1];
	int port = atoi(argv[2]);
	
	srand(time(NULL));
	signal(SIGPIPE, SIG_IGN);

	struct event_base *base = event_base_new();
	if(!base){
		fprintf(stderr, "event_base_new() error!\n");
		exit(0);
	}

	struct evhttp_connection *conn;
	struct evhttp_request *req;

	int num = 0;
	while(1){
		if(num % 100000 == 0){
			printf("press Enter to continue: ");
			getchar();
		}   
		if(num % 1000 == 1){
			log_debug("sub: %d", num);
		}
		conn = evhttp_connection_base_new(base, NULL, host, port + num % MAX_BIND_PORTS);
		req = evhttp_request_new(http_request_done, NULL);
		evhttp_request_set_chunked_cb(req, chunk_cb);

		char buf[128];
		snprintf(buf, sizeof(buf), "/sub?cname=%d", rand());
		evhttp_make_request(conn, req, EVHTTP_REQ_GET, buf);
		evhttp_connection_set_timeout(req->evcon, 864000);
		event_base_loop(base, EVLOOP_NONBLOCK);
    
		num ++; 
		usleep(1 * 1000);
	}   

	event_base_dispatch(base);
	
	log_debug("quit");
	return 0;
}

