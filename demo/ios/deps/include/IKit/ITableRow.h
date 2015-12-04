/*
 Copyright (c) 2014 ideawu. All rights reserved.
 Use of this source code is governed by a license that can be
 found in the LICENSE file.
 
 @author:  ideawu
 @website: http://www.cocoaui.com/
 */

#import "IView.h"

@interface ITableRow : IView

- (id)initWithNumberOfColumns:(NSUInteger)num;

- (void)column:(NSUInteger)column setText:(NSString *)text;
- (IView *)columnView:(NSUInteger)column;

@end
