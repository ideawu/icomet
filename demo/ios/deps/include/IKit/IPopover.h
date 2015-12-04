/*
 Copyright (c) 2014 ideawu. All rights reserved.
 Use of this source code is governed by a license that can be
 found in the LICENSE file.
 
 @author:  ideawu
 @website: http://www.cocoaui.com/
 */

#ifndef IKit_IPopover_h
#define IKit_IPopover_h

#import "IView.h"

@interface IPopover : IView

- (void)presentView:(IView *)view onView:(UIView *)containerView;
- (void)presentView:(IView *)view onViewController:(UIViewController *)controller;

@end

#endif
