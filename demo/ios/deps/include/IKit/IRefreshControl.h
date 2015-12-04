/*
 Copyright (c) 2014 ideawu. All rights reserved.
 Use of this source code is governed by a license that can be
 found in the LICENSE file.
 
 @author:  ideawu
 @website: http://www.cocoaui.com/
 */

#ifndef IKit_IRefreshControl_h
#define IKit_IRefreshControl_h

#import "IView.h"

@interface IRefreshControl : IView

@property (nonatomic, readonly) IView *indicatorView;
@property (nonatomic, readonly) IView *contentView;
@property (nonatomic) IRefreshState state;

- (void)setStateTextForNone:(NSString *)none maybe:(NSString *)maybe begin:(NSString *)begin;

@end

#endif
