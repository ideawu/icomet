//
//  ViewController.m
//  ios
//
//  Created by ideawu on 12/4/15.
//  Copyright © 2015 ideawu. All rights reserved.
//

#import "ViewController.h"
#import "IKit/IKit.h"
#include "curl/curl.h"

@interface ViewController (){
	CURL *_curl;
}
@property IView *mainView;
@end


// this function is called in a separated thread, it gets called when receive msg from icomet server
size_t icomet_callback(char *ptr, size_t size, size_t nmemb, void *userdata){
	const size_t sizeInBytes = size*nmemb;
	NSData *data = [[NSData alloc] initWithBytes:ptr length:sizeInBytes];
	NSString* s = [[NSString alloc] initWithData:data encoding:NSASCIIStringEncoding];
	NSLog(@"%@", s);
	// beware of multi-thread issue

	ViewController *controller = (__bridge ViewController *)userdata;
	dispatch_async(dispatch_get_main_queue(), ^{
		[controller.mainView.style set:@"background: #ff3"];
	});

	return sizeInBytes;
}

@implementation ViewController

- (void)viewDidLoad {
	[super viewDidLoad];
	self.navigationController.navigationBar.translucent = NO;

	NSString *xml = @""
	"<div style=\"width: 100%;\">"
	"	<div style=\"width: 100%; height: 100; background: #9cf; border-radius: 10; margin: 50;\">"
	"		<div style=\"width: 100; height: 50; float: center; background: #f66; margin-top: 10;\"></div>"
	"	</div>"
	"	<span style=\"float: center;\">Hello World!</span>"
	"</div>";
	_mainView = [IView viewFromXml:xml];
	[self.view addSubview:_mainView];

	///////////////////////////
	[self performSelectorInBackground:@selector(startStreaming) withObject:nil];
}

- (void)startStreaming{
	_curl = curl_easy_init();
	curl_easy_setopt(_curl, CURLOPT_URL, "http://127.0.0.1:8100/stream?cname=a&seq=1");
	curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1L);	// try not to use signals
	curl_easy_setopt(_curl, CURLOPT_USERAGENT, curl_version());	// set a default user agent
	curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, icomet_callback);
	curl_easy_setopt(_curl, CURLOPT_WRITEDATA, self);
	curl_easy_perform(_curl);
	curl_easy_cleanup(_curl);
}

@end
