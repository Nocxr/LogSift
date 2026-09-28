#import <Cocoa/Cocoa.h>

static bool gOpen = false;
static bool gToggleWatch = false;
static bool gToggleAutoCopy = false;
static bool gOpenLog = false;
static bool gCopy = false;
static bool gQuit = false;

static NSStatusItem* gItem = nil;
static NSMenuItem* gWatchItem = nil;
static NSMenuItem* gAutoCopyItem = nil;

@interface LogSiftStatusTarget : NSObject
- (void)openApp:(id)sender;
- (void)toggleWatch:(id)sender;
- (void)toggleAutoCopy:(id)sender;
- (void)openLog:(id)sender;
- (void)copyResults:(id)sender;
- (void)quitApp:(id)sender;
@end

@implementation LogSiftStatusTarget
- (void)openApp:(id)sender { (void)sender; gOpen = true; }
- (void)toggleWatch:(id)sender { (void)sender; gToggleWatch = true; }
- (void)toggleAutoCopy:(id)sender { (void)sender; gToggleAutoCopy = true; }
- (void)openLog:(id)sender { (void)sender; gOpenLog = true; }
- (void)copyResults:(id)sender { (void)sender; gCopy = true; }
- (void)quitApp:(id)sender { (void)sender; gQuit = true; }
@end

static LogSiftStatusTarget* gTarget = nil;

extern "C" void LogSiftMacTrayInit(void) {
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            LogSiftMacTrayInit();
        });
        return;
    }

    @autoreleasepool {
        if (gItem) return;

        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

        gTarget = [LogSiftStatusTarget new];
        gItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];

        NSStatusBarButton* button = gItem.button;
        button.toolTip = @"Log Sift";

        NSString* trayPath = [[NSBundle mainBundle] pathForResource:@"LogSiftTray" ofType:@"png"];
        NSImage* image = trayPath ? [[NSImage alloc] initWithContentsOfFile:trayPath] : nil;
        if (image) {
            [image setTemplate:YES];
            image.size = NSMakeSize(18.0, 18.0);
            button.image = image;
            button.imagePosition = NSImageOnly;
        } else if (@available(macOS 11.0, *)) {
            NSImage* fallback = [NSImage imageWithSystemSymbolName:@"line.3.horizontal.decrease.circle"
                                         accessibilityDescription:@"Log Sift"];
            [fallback setTemplate:YES];
            button.image = fallback;
            button.imagePosition = NSImageOnly;
        } else {
            button.title = @"LS";
        }

        NSMenu* menu = [NSMenu new];

        NSMenuItem* open = [[NSMenuItem alloc] initWithTitle:@"Open"
                                                     action:@selector(openApp:)
                                              keyEquivalent:@""];
        open.target = gTarget;
        [menu addItem:open];

        gWatchItem = [[NSMenuItem alloc] initWithTitle:@"Watch Clipboard"
                                               action:@selector(toggleWatch:)
                                        keyEquivalent:@""];
        gWatchItem.target = gTarget;
        gWatchItem.state = NSControlStateValueOn;
        [menu addItem:gWatchItem];

        gAutoCopyItem = [[NSMenuItem alloc] initWithTitle:@"Auto Copy"
                                                   action:@selector(toggleAutoCopy:)
                                            keyEquivalent:@""];
        gAutoCopyItem.target = gTarget;
        gAutoCopyItem.state = NSControlStateValueOff;
        [menu addItem:gAutoCopyItem];

        NSMenuItem* openLog = [[NSMenuItem alloc] initWithTitle:@"Open Log"
                                                        action:@selector(openLog:)
                                                 keyEquivalent:@""];
        openLog.target = gTarget;
        [menu addItem:openLog];

        [menu addItem:[NSMenuItem separatorItem]];

        NSMenuItem* quit = [[NSMenuItem alloc] initWithTitle:@"Exit"
                                                     action:@selector(quitApp:)
                                              keyEquivalent:@""];
        quit.target = gTarget;
        [menu addItem:quit];

        gItem.menu = menu;
    }
}

extern "C" bool LogSiftMacTrayTakeOpen(void) {
    const bool value = gOpen;
    gOpen = false;
    return value;
}

extern "C" bool LogSiftMacTrayTakeToggleWatch(void) {
    const bool value = gToggleWatch;
    gToggleWatch = false;
    return value;
}

extern "C" bool LogSiftMacTrayTakeToggleAutoCopy(void) {
    const bool value = gToggleAutoCopy;
    gToggleAutoCopy = false;
    return value;
}

extern "C" bool LogSiftMacTrayTakeOpenLog(void) {
    const bool value = gOpenLog;
    gOpenLog = false;
    return value;
}

extern "C" bool LogSiftMacTrayTakeCopy(void) {
    const bool value = gCopy;
    gCopy = false;
    return value;
}

extern "C" bool LogSiftMacTrayTakeQuit(void) {
    const bool value = gQuit;
    gQuit = false;
    return value;
}

extern "C" void LogSiftMacTraySetWatch(bool enabled) {
    if (gWatchItem) {
        gWatchItem.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
    }
}

extern "C" void LogSiftMacTraySetAutoCopy(bool enabled) {
    if (gAutoCopyItem) {
        gAutoCopyItem.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
    }
}

extern "C" long long LogSiftMacClipboardChangeCount(void) {
    return (long long)[[NSPasteboard generalPasteboard] changeCount];
}


static NSString* LogSiftLaunchAgentPath(void) {
    NSString* launchAgents = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/LaunchAgents"];
    return [launchAgents stringByAppendingPathComponent:@"com.nocxr.logsift.plist"];
}

extern "C" bool LogSiftMacGetStartAtLogin(void) {
    return [[NSFileManager defaultManager] fileExistsAtPath:LogSiftLaunchAgentPath()];
}

extern "C" bool LogSiftMacSetStartAtLogin(bool enabled) {
    NSFileManager* fm = [NSFileManager defaultManager];
    NSString* plistPath = LogSiftLaunchAgentPath();
    if (!enabled) {
        if (![fm fileExistsAtPath:plistPath]) return true;
        return [fm removeItemAtPath:plistPath error:nil];
    }

    NSString* launchAgents = [plistPath stringByDeletingLastPathComponent];
    if (![fm createDirectoryAtPath:launchAgents
       withIntermediateDirectories:YES
                        attributes:nil
                             error:nil]) {
        return false;
    }

    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    if (!bundlePath || bundlePath.length == 0) return false;

    NSDictionary* plist = @{
        @"Label": @"com.nocxr.logsift",
        @"ProgramArguments": @[@"/usr/bin/open", @"-g", bundlePath, @"--args", @"--background"],
        @"RunAtLoad": @YES
    };
    return [plist writeToFile:plistPath atomically:YES];
}
