/*
Copyright (c) 2012-2016 The icomet Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "../build.h"
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
#include "util/daemon.h"
#include "util/ip_filter.h"
#include "channel.h"
#include "server.h"
#include "server_config.h"

// for testing
#define MAX_BIND_PORTS 1

int ServerConfig::max_channels					= 0;
int ServerConfig::max_subscribers_per_channel	= 0;
int ServerConfig::polling_timeout				= 0;
int ServerConfig::polling_idles					= 0;
int ServerConfig::channel_buffer_size			= 0;
int ServerConfig::channel_timeout				= 0;
int ServerConfig::channel_idles					= 0;
/*
std::string ServerConfig::iframe_header			= "";
std::string ServerConfig::iframe_chunk_prefix	= "";
std::string ServerConfig::iframe_chunk_suffix	= "";
*/

static inline
int parse_ip_port(const char *addr, std::string *ip, int *port){
	char *sep = (char *)strchr(addr, ':');
	if(!sep){
		return -1;
	}
	ip->assign(addr, sep - addr);
	*port = atoi(sep + 1);
	if(*port < 0 || *port > 65535){
		return -1;
	}
	return 0;
}

struct AppArgs{
	bool is_daemon;
	std::string pidfile;
	std::string conf_file;
	std::string start_opt;

	AppArgs(){
		is_daemon = false;
		start_opt = "start";
	}
};

AppArgs app_args;
Server *serv = NULL;
Config *conf = NULL;
IpFilter *ip_filter = NULL;
struct event_base *evbase = NULL;
struct evhttp *admin_http = NULL;
struct evhttp *front_http = NULL;
struct event *timer_event = NULL;
struct event *sigint_event = NULL;
struct event *sigterm_event = NULL;

void welcome();
void usage(int argc, char **argv);
void parse_args(int argc, char **argv);
void init(int argc, char **argv);

int read_pid();
void write_pid();
void check_pidfile();
void remove_pidfile();
void kill_process();

void signal_cb(evutil_socket_t sig, short events, void *user_data){
	event_base_loopbreak(evbase);
}

void poll_handler(struct evhttp_request *req, void *arg){
	serv->poll(req);
}

void iframe_handler(struct evhttp_request *req, void *arg){
	serv->iframe(req);
}

void stream_handler(struct evhttp_request *req, void *arg){
	serv->stream(req);
}

void sse_handler(struct evhttp_request *req, void *arg){
	serv->sse(req);
}

void ping_handler(struct evhttp_request *req, void *arg){
	serv->ping(req);
}

#define CHECK_AUTH() \
	do{ \
		evhttp_add_header(req->output_headers, "Server", ICOMET_HTTP_HEADER_SERVER); \
		bool pass = ip_filter->check_pass(req->remote_host); \
		if(!pass){ \
			log_info("admin deny %s:%d", req->remote_host, req->remote_port); \
			evhttp_add_header(req->output_headers, "Connection", "Close"); \
			evhttp_send_reply(req, 403, "Forbidden", NULL); \
			return; \
		} \
	}while(0)

void pub_handler(struct evhttp_request *req, void *arg){
	CHECK_AUTH();
	serv->pub(req, true);
}

void push_handler(struct evhttp_request *req, void *arg){
	CHECK_AUTH();
	serv->pub(req, false);
}

void broadcast_handler(struct evhttp_request *req, void *arg){
	CHECK_AUTH();
	serv->broadcast(req);
}

void sign_handler(struct evhttp_request *req, void *arg){
	CHECK_AUTH();
	serv->sign(req);
}

void close_handler(struct evhttp_request *req, void *arg){
	CHECK_AUTH();
	serv->close(req);
}

void clear_handler(struct evhttp_request *req, void *arg){
	CHECK_AUTH();
	serv->clear(req);
}

void info_handler(struct evhttp_request *req, void *arg){
	CHECK_AUTH();
	serv->info(req);
}

void check_handler(struct evhttp_request *req, void *arg){
	CHECK_AUTH();
	serv->check(req);
}

void psub_handler(struct evhttp_request *req, void *arg){
	CHECK_AUTH();
	serv->psub(req);
}

#undef CHECK_AUTH

void timer_cb(evutil_socket_t sig, short events, void *user_data){
	rand();
	serv->check_timeout();
}

void accept_error_cb(struct evconnlistener *lis, void *ptr){
	// do nothing
}

int main(int argc, char **argv){
	welcome();
	parse_args(argc, argv);
	init(argc, argv);
	
	ServerConfig::max_channels = conf->get_num("front.max_channels");
	ServerConfig::max_subscribers_per_channel = conf->get_num("front.max_subscribers_per_channel");
	ServerConfig::channel_buffer_size = conf->get_num("front.channel_buffer_size");
	ServerConfig::channel_timeout = conf->get_num("front.channel_timeout");
	ServerConfig::polling_timeout = conf->get_num("front.polling_timeout");
	if(ServerConfig::polling_timeout <= 0){
		log_fatal("Invalid polling_timeout!");
		exit(0);
	}
	if(ServerConfig::channel_timeout <= 0){
		ServerConfig::channel_timeout = (int)(0.5 * ServerConfig::polling_timeout);
	}
	
	ServerConfig::polling_idles = ServerConfig::polling_timeout / CHANNEL_CHECK_INTERVAL;
	ServerConfig::channel_idles = ServerConfig::channel_timeout / CHANNEL_CHECK_INTERVAL;
	
	serv = new Server();
	ip_filter = new IpFilter();

	{
		// /pub?cname=abc&content=hi
		// content must be json encoded string without leading quote and trailing quote
		// TODO: multi_pub
		evhttp_set_cb(admin_http, "/pub", pub_handler, NULL);
		// pub raw content(not json encoded)
		evhttp_set_cb(admin_http, "/push", push_handler, NULL);
		evhttp_set_cb(admin_http, "/broadcast", broadcast_handler, NULL);
		// 分配通道, 返回通道的id和token
		// /sign?cname=abc[&expires=60]
		// wait 60 seconds to expire before any sub
		evhttp_set_cb(admin_http, "/sign", sign_handler, NULL);
		// 销毁通道
		// /close?cname=abc
		evhttp_set_cb(admin_http, "/close", close_handler, NULL);
		// 清除通道的消息
		// /clear?cname=abc
		evhttp_set_cb(admin_http, "/clear", clear_handler, NULL);
		// 获取通道的信息
		// /info?[cname=abc], or TODO: /info?cname=a,b,c
		evhttp_set_cb(admin_http, "/info", info_handler, NULL);
		// 判断通道是否处于被订阅状态(所有订阅者断开连接一定时间后, 通道才转为空闲状态)
		// /check?cname=abc, or TODO: /check?cname=a,b,c
		evhttp_set_cb(admin_http, "/check", check_handler, NULL);
		
		// 订阅通道的状态变化信息, 如创建通道(第一个订阅者连接时), 关闭通道.
		// 通过 endless chunk 返回.
		evhttp_set_cb(admin_http, "/psub", psub_handler, NULL);
	
		std::string admin_ip;
		int admin_port = 0;
		parse_ip_port(conf->get_str("admin.listen"), &admin_ip, &admin_port);
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
		// /sub?cname=abc&cb=jsonp&token=&seq=123&noop=123
		evhttp_set_cb(front_http, "/sub", poll_handler, NULL);
		evhttp_set_cb(front_http, "/poll", poll_handler, NULL);
		// forever iframe
		evhttp_set_cb(front_http, "/iframe", iframe_handler, NULL);
		// http endless chunk
		evhttp_set_cb(front_http, "/stream", stream_handler, NULL);
		// http5 Server Send Event
		evhttp_set_cb(front_http, "/sse", sse_handler, NULL);
		// /ping?cb=jsonp
		evhttp_set_cb(front_http, "/ping", ping_handler, NULL);

		std::string front_ip;
		int front_port = 0;
		parse_ip_port(conf->get_str("front.listen"), &front_ip, &front_port);
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

		std::string auth = conf->get_str("front.auth");
		log_info("    auth %s", auth.c_str());
		log_info("    max_channels %d", ServerConfig::max_channels);
		log_info("    max_subscribers_per_channel %d", ServerConfig::max_subscribers_per_channel);
		log_info("    channel_buffer_size %d", ServerConfig::channel_buffer_size);
		log_info("    channel_timeout %d", ServerConfig::channel_timeout);
		log_info("    polling_timeout %d", ServerConfig::polling_timeout);
		if(auth == "token"){
			serv->auth = Server::AUTH_TOKEN;
		}
	}

	write_pid();
	log_info("icomet-server started");
	event_base_dispatch(evbase);
	remove_pidfile();

	event_free(timer_event);
	event_free(sigint_event);
	event_free(sigterm_event);
	evhttp_free(admin_http);
	evhttp_free(front_http);
	event_base_free(evbase);

	delete serv;
	delete conf;
	delete ip_filter;

	log_info("icomet exit");
	return 0;
}

void welcome(){
	printf("icomet-server %s\n", ICOMET_VERSION);
	printf("Copyright (c) 2013-2016 ideawu.com\n");
	printf("\n");
}

void usage(int argc, char **argv){
	printf("Usage:\n");
	printf("    %s [-d] /path/to/icomet.conf [-s start|stop|restart]\n", argv[0]);
	printf("Options:\n");
	printf("    -d    run as daemon\n");
	printf("    -s    option to start|stop|restart the server\n");
	printf("    -h    show this message\n");
}

void parse_args(int argc, char **argv){
	for(int i=1; i<argc; i++){
		std::string arg = argv[i];
		if(arg == "-d"){
			app_args.is_daemon = true;
		}else if(arg == "-v"){
			exit(0);
		}else if(arg == "-h"){
			usage(argc, argv);
			exit(0);
		}else if(arg == "-s"){
			if(argc > i + 1){
				i ++;
				app_args.start_opt = argv[i];
			}else{
				usage(argc, argv);
				exit(1);
			}
			if(app_args.start_opt != "start" && app_args.start_opt != "stop" && app_args.start_opt != "restart"){
				usage(argc, argv);
				fprintf(stderr, "Error: bad argument: '%s'\n", app_args.start_opt.c_str());
				exit(1);
			}
		}else{
			app_args.conf_file = argv[i];
		}
	}

	if(app_args.conf_file.empty()){
		usage(argc, argv);
		exit(1);
	}
}

void init(int argc, char **argv){
	signal(SIGPIPE, SIG_IGN);

	{
		struct timeval tv;
		if(gettimeofday(&tv, NULL) == -1){
			srand(time(NULL) + getpid());
		}else{
			srand(tv.tv_sec + tv.tv_usec + getpid());
		}
	}

	if(!is_file(app_args.conf_file.c_str())){
		fprintf(stderr, "'%s' is not a file or not exists!\n", app_args.conf_file.c_str());
		exit(1);
	}
	conf = Config::load(app_args.conf_file.c_str());
	if(!conf){
		fprintf(stderr, "error loading conf file: '%s'\n", app_args.conf_file.c_str());
		exit(1);
	}
	{
		std::string conf_dir = real_dirname(app_args.conf_file.c_str());
		if(chdir(conf_dir.c_str()) == -1){
			fprintf(stderr, "error chdir: %s\n", conf_dir.c_str());
			exit(1);
		}
	}

	app_args.pidfile = conf->get_str("pidfile");

	if(app_args.start_opt == "stop"){
		kill_process();
		exit(0);
	}
	if(app_args.start_opt == "restart"){
		if(file_exists(app_args.pidfile)){
			kill_process();
		}
	}
	check_pidfile();

	std::string log_output;
	int log_rotate_size = 0;
	{ // logger
		int log_level = Logger::get_level(conf->get_str("logger.level"));
		log_rotate_size = conf->get_num("logger.rotate.size");
		if(log_rotate_size < 1024 * 1024){
			log_rotate_size = 1024 * 1024;
		}
		log_output = conf->get_str("logger.output");
		if(log_output == ""){
			log_output = "stdout";
		}
		if(log_open(log_output.c_str(), log_level, true, log_rotate_size) == -1){
			fprintf(stderr, "error open log file: %s\n", log_output.c_str());
			exit(0);
		}
	}

	log_info("starting icomet-server %s...", ICOMET_VERSION);
	log_info("conf_file       : %s", app_args.conf_file.c_str());
	log_info("log_level       : %s", conf->get_str("logger.level"));
	log_info("log_output      : %s", log_output.c_str());
	log_info("log_rotate_size : %d", log_rotate_size);

	if(app_args.is_daemon){
		daemonize();
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
		tv.tv_sec = CHANNEL_CHECK_INTERVAL;
		tv.tv_usec = 0;
		if(!timer_event || evtimer_add(timer_event, &tv)<0){
			fprintf(stderr, "Could not create/add a timer event!\n");
			exit(0);
		}
	}
}

void check_pidfile(){
	if(app_args.pidfile.size()){
		if(access(app_args.pidfile.c_str(), F_OK) == 0){
			fprintf(stderr, "Fatal error!\nPidfile %s already exists!\n"
				"Kill the running process before you run this command,\n"
				"or use '-s restart' option to restart the server.\n",
				app_args.pidfile.c_str());
			exit(1);
		}
	}
}

int read_pid(){
	if(app_args.pidfile.empty()){
		return -1;
	}
	std::string s;
	file_get_contents(app_args.pidfile, &s);
	if(s.empty()){
		return -1;
	}
	return str_to_int(s);
}

void write_pid(){
	if(app_args.pidfile.empty()){
		return;
	}
	int pid = (int)getpid();
	std::string s = str(pid);
	log_info("pidfile: %s, pid: %d", app_args.pidfile.c_str(), pid);
	int ret = file_put_contents(app_args.pidfile, s);
	if(ret == -1){
		log_error("Failed to write pidfile '%s'(%s)", app_args.pidfile.c_str(), strerror(errno));
		exit(1);
	}
}

void remove_pidfile(){
	if(app_args.pidfile.size()){
		remove(app_args.pidfile.c_str());
	}
}

void kill_process(){
	int pid = read_pid();
	if(pid == -1){
		fprintf(stderr, "could not read pidfile: %s(%s)\n", app_args.pidfile.c_str(), strerror(errno));
		exit(1);
	}
	if(kill(pid, 0) == -1 && errno == ESRCH){
		fprintf(stderr, "process: %d not running\n", pid);
		remove_pidfile();
		return;
	}
	int ret = kill(pid, SIGTERM);
	if(ret == -1){
		fprintf(stderr, "could not kill process: %d(%s)\n", pid, strerror(errno));
		exit(1);
	}
	
	while(file_exists(app_args.pidfile)){
		usleep(100 * 1000);
	}
}
