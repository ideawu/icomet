//
//  ISwitch.h
//  IKit
//
//  Created by ideawu on 7/29/15.
//  Copyright (c) 2015 ideawu. All rights reserved.
//

#import "IView.h"

@interface ISwitch : IView

@property (nonatomic, readonly) UISwitch *uiswitch;

@property (nonatomic,getter=isOn) BOOL on;

- (void)setOn:(BOOL)on animated:(BOOL)animated; // does not send action

@end
