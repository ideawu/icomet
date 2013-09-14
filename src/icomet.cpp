#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <evhttp.h>
#include <event2/event.h>
#include <event2/http.h>
#include "util/log.h"
#include "channel.h"
#include "server.h"

Server *serv;

void signal_cb(evutil_socket_t sig, short events, void *user_data){
	struct event_base *base = (struct event_base *)user_data;
	event_base_loopbreak(base);
}

void sub_handler(struct evhttp_request *req, void *arg){
	serv->sub(req);
}

void pub_handler(struct evhttp_request *req, void *arg){
	serv->pub(req);
}

void timer_cb(evutil_socket_t sig, short events, void *user_data){
	serv->heartbeat();
}

int main(int argc, char **argv){
	signal(SIGPIPE, SIG_IGN);

	struct event_base *base;
	struct evhttp *http;
	struct event *sigint_event;
	struct event *sigterm_event;
	struct event *timer_event;
	unsigned short port = 8000;

	base = event_base_new();
	if(!base){
		fprintf(stderr, "event_base_new() error!\n");
		exit(0);
	}
	http = evhttp_new(base);
	
	sigint_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
	if(!sigint_event || event_add(sigint_event, NULL)<0){
		fprintf(stderr, "Could not create/add a signal event!\n");
		exit(0);
	}
	sigterm_event = evsignal_new(base, SIGTERM, signal_cb, (void *)base);
	if(!sigterm_event || event_add(sigterm_event, NULL)<0){
		fprintf(stderr, "Could not create/add a signal event!\n");
		exit(0);
	}
	timer_event = event_new(base, -1, EV_PERSIST, timer_cb, NULL);
	{
		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		if(!timer_event || evtimer_add(timer_event, &tv)<0){
			fprintf(stderr, "Could not create/add a timer event!\n");
			exit(0);
		}
	}

	// /sub?cb=js_callback&id=123&token=&[last_seq=123]
	evhttp_set_cb(http, "/sub", sub_handler, NULL);
	// /pub?id=123&content=hi or /pub?uid=xxx&content=hi
	evhttp_set_cb(http, "/pub", pub_handler, NULL);
	// 分配通道, 返回通道的id和token
	// /sign?uid=xxx
	// 销毁通道
	// /close?id=123 or /close?uid=xxx
	// 获取通道的信息
	// /stat?id=123 or /stat?uid=xxx
	// 判断通道是否处于被订阅状态(所有订阅者断开连接一定时间后, 通道才转为空闲状态)
	// /check?id=123,234
	
	//evhttp_set_gencb(http, request_handler, NULL);
	
	struct evhttp_bound_socket *handle;
	handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
	printf("server listen at 127.0.0.1:%d\n", port);

	serv = new Server();
	
	event_base_dispatch(base);
	log_debug("event dispatch stopped");
	
	event_free(timer_event);
	event_free(sigint_event);
	event_free(sigterm_event);
	evhttp_free(http);
	event_base_free(base);

	delete serv;

	log_debug("quit");

	return 0;
}

