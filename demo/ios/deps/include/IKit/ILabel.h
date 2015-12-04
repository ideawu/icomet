/*
 Copyright (c) 2014 ideawu. All rights reserved.
 Use of this source code is governed by a license that can be
 found in the LICENSE file.
 
 @author:  ideawu
 @website: http://www.cocoaui.com/
 */

#import "IView.h"

@interface ILabel : IView

@property (nonatomic, readonly) UILabel *label;
@property (nonatomic) NSString *text;
@property (nonatomic) NSAttributedString *attributedText;

+ (ILabel *)labelWithText:(NSString *)text;

@end
