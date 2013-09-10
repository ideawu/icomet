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
#include "util/log.h"
#include "channel.h"
#include "server.h"

Server serv;

void sub_handler(struct evhttp_request *req, void *arg){
	serv.sub(req);
}

void pub_handler(struct evhttp_request *req, void *arg){
	serv.pub(req);
}

int main(int argc, char **argv){
	signal(SIGPIPE, SIG_IGN);

	struct event_base *base;
	struct evhttp *http;
	struct evhttp_bound_socket *handle;
	unsigned short port = 8000;
	

	base = event_base_new();
	http = evhttp_new(base);
	
	// /sub?id=123
	evhttp_set_cb(http, "/sub", sub_handler, NULL);
	// /pub?id=123&content=hi
	evhttp_set_cb(http, "/pub", pub_handler, NULL);
	// /stat?id=123
	//evhttp_set_cb(http, "/stat", pub_handler, NULL);
	// 判断通道是否处于被订阅状态(所有订阅者断开连接一定时间后, 通道才转为空闲状态)
	// /check?id=123,234
	//evhttp_set_cb(http, "/check", pub_handler, NULL);
	//evhttp_set_gencb(http, request_handler, NULL);
	
	handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
	printf("server listen at 127.0.0.1:%d\n", port);
	event_base_dispatch(base);

	return 0;
}

