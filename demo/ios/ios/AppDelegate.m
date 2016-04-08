//
//  AppDelegate.m
//  ios
//
//  Created by ideawu on 12/4/15.
//  Copyright © 2015 ideawu. All rights reserved.
//

#import "AppDelegate.h"
#import "ViewController.h"

@interface AppDelegate ()

@property ViewController *viewController;

@end

@implementation AppDelegate


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	NSLog(@"%s", __func__);
	
	self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
	//UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:table];
	UINavigationController *nav = [[UINavigationController alloc] init];
	nav.navigationBar.titleTextAttributes = @{NSForegroundColorAttributeName: [UIColor blackColor]};
	nav.navigationBar.translucent = NO;
	nav.view.backgroundColor = [UIColor groupTableViewBackgroundColor];
	[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
	
	self.window.rootViewController = nav;
	[self.window makeKeyAndVisible];
	
	_viewController = [[ViewController alloc] init];
	[nav pushViewController:_viewController animated:YES];

	return YES;
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	NSLog(@"%s", __func__);
	[_viewController start];
	// Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillResignActive:(UIApplication *)application {
	NSLog(@"%s", __func__);
	[_viewController shutdown];
	// Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
	// Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
	NSLog(@"%s", __func__);
	// Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
	// If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
	NSLog(@"%s", __func__);
	// Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationWillTerminate:(UIApplication *)application {
	NSLog(@"%s", __func__);
	// Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end
