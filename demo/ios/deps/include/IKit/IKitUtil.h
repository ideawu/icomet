/*
 Copyright (c) 2014 ideawu. All rights reserved.
 Use of this source code is governed by a license that can be
 found in the LICENSE file.
 
 @author:  ideawu
 @website: http://www.cocoaui.com/
 */

#ifndef IKit_IKitUtil_h
#define IKit_IKitUtil_h

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface IKitUtil : NSObject

+ (UIColor *) colorFromHex:(NSString *)hex;

+ (BOOL)isHTML:(NSString *)str;
+ (BOOL)isHttpUrl:(NSString *)src;

+ (NSArray *)parsePath:(NSString *)path;
// combine basePath + src, src may be URL or absolute file path
+ (NSString *)buildPath:(NSString *)basePath src:(NSString *)src;

+ (UIImage *)loadImageFromPath:(NSString *)path;

+ (BOOL)isDataURI:(NSString *)src;
+ (UIImage *)loadImageFromDataURI:(NSString *)src;

@end

#endif
