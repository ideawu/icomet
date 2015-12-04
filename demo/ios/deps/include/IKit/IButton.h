/*
 Copyright (c) 2014 ideawu. All rights reserved.
 Use of this source code is governed by a license that can be
 found in the LICENSE file.
 
 @author:  ideawu
 @website: http://www.cocoaui.com/
 */

#import "IView.h"

@interface IButton : IView

@property (nonatomic, readonly) UIButton *button;
@property (nonatomic) NSString *text;

+ (IButton *)buttonWithText:(NSString *)text;

@end
