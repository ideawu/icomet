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
void write_pidfile();
void check_pidfile();
void remove_pidfile();

void welcome(){
	printf("icomet comet-server %s\n", ICOMET_VERSION);
	printf("Copyright (c) 2013 ideawu.com\n");
	printf("\n");
}

void usage(int argc, char **argv){
	printf("Usage:\n");
	printf("    %s [-d] /path/to/comet.conf\n", argv[0]);
	printf("Options:\n");
	printf("    -d    run as daemon\n");
}

void signal_cb(evutil_socket_t sig, short events, void *user_data){
	event_base_loopbreak(evbase);
}

void sub_handler(struct evhttp_request *req, void *arg){
	serv->sub(req);
}

void ping_handler(struct evhttp_request *req, void *arg){
	serv->ping(req);
}

#define CHECK_AUTH() \
	do{ \
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
	serv->pub(req);
}

void sign_handler(struct evhttp_request *req, void *arg){
	CHECK_AUTH();
	serv->sign(req);
}

void close_handler(struct evhttp_request *req, void *arg){
	CHECK_AUTH();
	serv->close(req);
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
	init(argc, argv);
	
	ServerConfig::max_channels = conf->get_num("front.max_channels");
	ServerConfig::max_subscribers_per_channel = conf->get_num("front.max_subscribers_per_channel");
	ServerConfig::polling_timeout = conf->get_num("front.polling_timeout");
	ServerConfig::channel_buffer_size = conf->get_num("front.channel_buffer_size");
	ServerConfig::channel_timeout = 0.5 * ServerConfig::polling_timeout;
	
	ServerConfig::polling_idles = ServerConfig::polling_timeout / CHANNEL_CHECK_INTERVAL;
	ServerConfig::channel_idles = ServerConfig::channel_timeout / CHANNEL_CHECK_INTERVAL;
		
	serv = new Server();
	ip_filter = new IpFilter();

	{
		// /pub?cname=abc&content=hi
		// content must be json encoded string without leading quote and trailing quote
		evhttp_set_cb(admin_http, "/pub", pub_handler, NULL);
		// 分配通道, 返回通道的id和token
		// /sign?cname=abc&[expires=60]
		// wait 60 seconds to expire before any sub
		evhttp_set_cb(admin_http, "/sign", sign_handler, NULL);
		// 销毁通道
		// /close?cname=abc
		evhttp_set_cb(admin_http, "/close", close_handler, NULL);
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
		// TODO: sub?cname=abc&cb=jsonp&token=&seq=123&noop=123
		evhttp_set_cb(front_http, "/sub", sub_handler, NULL);
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
		log_info("    polling_timeout %d", ServerConfig::polling_timeout);
		if(auth == "token"){
			serv->auth = Server::AUTH_TOKEN;
		}
	}
	
	write_pidfile();
	log_info("icomet started");
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

void init(int argc, char **argv){
	if(argc < 2){
		usage(argc, argv);
		exit(0);
	}
	signal(SIGPIPE, SIG_IGN);

	{
		struct timeval tv;
		if(gettimeofday(&tv, NULL) == -1){
			srand(time(NULL) + getpid());
		}else{
			srand(tv.tv_sec + tv.tv_usec + getpid());
		}
	}

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

	check_pidfile();
	if(is_daemon){
		daemonize();
	}

	log_info("starting icomet %s...", ICOMET_VERSION);
	log_info("config file: %s", conf_file);
	log_info("log_level       : %s", conf->get_str("logger.level"));
	log_info("log_output      : %s", log_output.c_str());
	log_info("log_rotate_size : %d", log_rotate_size);


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

void write_pidfile(){
	const char *pidfile = conf->get_str("pidfile");
	if(strlen(pidfile)){
		FILE *fp = fopen(pidfile, "w");
		if(!fp){
			log_error("Failed to open pidfile '%s': %s", pidfile, strerror(errno));
			exit(0);
		}
		char buf[128];
		pid_t pid = getpid();
		snprintf(buf, sizeof(buf), "%d", pid);
		log_info("pidfile: %s, pid: %d", pidfile, pid);
		fwrite(buf, 1, strlen(buf), fp);
		fclose(fp);
	}
}

void check_pidfile(){
	const char *pidfile = conf->get_str("pidfile");
	if(strlen(pidfile)){
		if(access(pidfile, F_OK) == 0){
			fprintf(stderr, "Fatal error!\nPidfile %s already exists!\n"
				"You must kill the process and then "
				"remove this file before starting icomet.\n", pidfile);
			exit(0);
		}
	}
}

void remove_pidfile(){
	const char *pidfile = conf->get_str("pidfile");
	if(strlen(pidfile)){
		remove(pidfile);
	}
}
