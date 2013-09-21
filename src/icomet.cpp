#include "../config.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <evhttp.h>
#include <event2/event.h>
#include <event2/listener.h>
#include "util/log.h"
#include "util/file.h"
#include "util/config.h"
#include "util/strings.h"
#include "channel.h"
#include "server.h"
#include "ip_filter.h"

#define MAX_BIND_PORTS 2

Server *serv = NULL;
Config *conf = NULL;
IpFilter *ip_filter = NULL;
struct event_base *evbase = NULL;
struct evhttp *admin_http = NULL;
struct evhttp *front_http = NULL;
struct event *timer_event = NULL;
struct event *sigint_event = NULL;
struct event *sigterm_event = NULL;

void init(int argc, char **argv);

void welcome(){
	printf("icomet server %s\n", ICOMET_VERSION);
	printf("Copyright (c) 2013 ideawu.com\n");
	printf("\n");
}

void usage(int argc, char **argv){
	printf("Usage:\n");
	printf("    %s /path/to/icomet.conf\n", argv[0]);
	//printf("Options:\n");
	//printf("    -d    run as daemon\n");
}


void signal_cb(evutil_socket_t sig, short events, void *user_data){
	event_base_loopbreak(evbase);
}

void sub_handler(struct evhttp_request *req, void *arg){
	serv->sub(req);
}

void pub_handler(struct evhttp_request *req, void *arg){
	bool pass = ip_filter->check_pass(req->remote_host);
	if(pass){
		serv->pub(req);
	}else{
		log_info("admin deny %s:%d", req->remote_host, req->remote_port);
		evhttp_add_header(req->output_headers, "Connection", "Close");
		evhttp_send_reply(req, 403, "Forbidden", NULL);
	}
}

void timer_cb(evutil_socket_t sig, short events, void *user_data){
	serv->check_timeout();
}

void accept_error_cb(struct evconnlistener *lis, void *ptr){
	// do nothing
}

int main(int argc, char **argv){	
	welcome();
	init(argc, argv);

	log_info("starting icomet %s...", ICOMET_VERSION);

	std::string admin_ip;
	std::string front_ip;
	int admin_port;
	int front_port;
	
	parse_ip_port(conf->get_str("admin.listen"), &admin_ip, &admin_port);
	parse_ip_port(conf->get_str("front.listen"), &front_ip, &front_port);

	{
		// /pub?id=123&content=hi or /pub?uid=xxx&content=hi
		// content must be json encoded string without leading and trailing quotes
		evhttp_set_cb(admin_http, "/pub", pub_handler, NULL);
		// 分配通道, 返回通道的id和token
		// /sign?uid=xxx
		// 销毁通道
		// /close?id=123 or /close?uid=xxx
		// 获取通道的信息
		// /stat?id=123 or /stat?uid=xxx
		// 判断通道是否处于被订阅状态(所有订阅者断开连接一定时间后, 通道才转为空闲状态)
		// /check?id=123,234
		
		struct evhttp_bound_socket *handle;
		handle = evhttp_bind_socket_with_handle(admin_http, admin_ip.c_str(), admin_port);
		if(!handle){
			log_fatal("bind admin_port %d error! %s", admin_port, strerror(errno));
			exit(0);
		}
		log_info("admin server listen on %s:%d", admin_ip.c_str(), admin_port);

		struct evconnlistener *listener = evhttp_bound_socket_get_listener(handle);
		evconnlistener_set_error_cb(listener, accept_error_cb);
		// TODO: modify libevent, add evhttp_set_accept_cb()
	}

	// init admin ip_filter
	{
		Config *cc = (Config *)conf->get("admin");
		if(cc != NULL){
			std::vector<Config *> *children = &cc->children;
			std::vector<Config *>::iterator it;
			for(it = children->begin(); it != children->end(); it++){
				if((*it)->key != "allow"){
					continue;
				}
				const char *ip = (*it)->str();
				log_info("    allow %s", ip);
				ip_filter->add_allow(ip);
			}
		}
	}
	{
		Config *cc = (Config *)conf->get("admin");
		if(cc != NULL){
			std::vector<Config *> *children = &cc->children;
			std::vector<Config *>::iterator it;
			for(it = children->begin(); it != children->end(); it++){
				if((*it)->key != "deny"){
					continue;
				}
				const char *ip = (*it)->str();
				log_info("    deny %s", ip);
				ip_filter->add_deny(ip);
			}
		}
	}

	{
		// /sub?cb=jsonp&id=123&token=&[last_seq=123]
		evhttp_set_cb(front_http, "/sub", sub_handler, NULL);	
		for(int i=0; i<MAX_BIND_PORTS; i++){
			int port = front_port + i;
			
			struct evhttp_bound_socket *handle;
			handle = evhttp_bind_socket_with_handle(front_http, front_ip.c_str(), port);
			if(!handle){
				log_fatal("bind front_port %d error! %s", port, strerror(errno));
				exit(0);
			}
			log_info("front server listen on %s:%d", front_ip.c_str(), port);

			struct evconnlistener *listener = evhttp_bound_socket_get_listener(handle);
			evconnlistener_set_error_cb(listener, accept_error_cb);
		}
		
		// /ping?cb=jsonp
	}

	log_info("icomet started");
	event_base_dispatch(evbase);
	log_info("icomet exit");

	event_free(timer_event);
	event_free(sigint_event);
	event_free(sigterm_event);
	evhttp_free(admin_http);
	evhttp_free(front_http);
	event_base_free(evbase);
	delete serv;
	delete conf;
	delete ip_filter;

	return 0;
}

void init(int argc, char **argv){
	if(argc < 2){
		usage(argc, argv);
		exit(0);
	}
	signal(SIGPIPE, SIG_IGN);

	bool is_daemon = false;
	const char *conf_file = NULL;
	for(int i=1; i<argc; i++){
		if(strcmp(argv[i], "-d") == 0){
			is_daemon = true;
		}else{
			conf_file = argv[i];
		}
	}

	if(conf_file == NULL){
		usage(argc, argv);
		exit(0);
	}

	if(!is_file(conf_file)){
		fprintf(stderr, "'%s' is not a file or not exists!\n", conf_file);
		exit(0);
	}

	conf = Config::load(conf_file);
	if(!conf){
		fprintf(stderr, "error loading conf file: '%s'\n", conf_file);
		exit(0);
	}
	{
		std::string conf_dir = real_dirname(conf_file);
		if(chdir(conf_dir.c_str()) == -1){
			fprintf(stderr, "error chdir: %s\n", conf_dir.c_str());
			exit(0);
		}
	}

	evbase = event_base_new();
	if(!evbase){
		fprintf(stderr, "create evbase error!\n");
		exit(0);
	}
	admin_http = evhttp_new(evbase);
	if(!admin_http){
		fprintf(stderr, "create admin_http error!\n");
		exit(0);
	}
	front_http = evhttp_new(evbase);
	if(!front_http){
		fprintf(stderr, "create front_http error!\n");
		exit(0);
	}
	
	sigint_event = evsignal_new(evbase, SIGINT, signal_cb, NULL);
	if(!sigint_event || event_add(sigint_event, NULL)<0){
		fprintf(stderr, "Could not create/add a signal event!\n");
		exit(0);
	}
	sigterm_event = evsignal_new(evbase, SIGTERM, signal_cb, NULL);
	if(!sigterm_event || event_add(sigterm_event, NULL)<0){
		fprintf(stderr, "Could not create/add a signal event!\n");
		exit(0);
	}
	timer_event = event_new(evbase, -1, EV_PERSIST, timer_cb, NULL);
	{
		struct timeval tv;
		tv.tv_sec = SUB_CHECK_INTERVAL;
		tv.tv_usec = 0;
		if(!timer_event || evtimer_add(timer_event, &tv)<0){
			fprintf(stderr, "Could not create/add a timer event!\n");
			exit(0);
		}
	}

	ip_filter = new IpFilter();
	serv = new Server();
}
