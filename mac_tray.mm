#import <Cocoa/Cocoa.h>

static bool gOpen = false;
static bool gToggleWatch = false;
static bool gCopy = false;
static bool gQuit = false;

static NSStatusItem* gItem = nil;
static NSMenuItem* gWatchItem = nil;

@interface LogSiftStatusTarget : NSObject
- (void)openApp:(id)sender;
- (void)toggleWatch:(id)sender;
- (void)copyResults:(id)sender;
- (void)quitApp:(id)sender;
@end

@implementation LogSiftStatusTarget
- (void)openApp:(id)sender { (void)sender; gOpen = true; }
- (void)toggleWatch:(id)sender { (void)sender; gToggleWatch = true; }
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
            image.template = YES;
            image.size = NSMakeSize(18.0, 18.0);
            button.image = image;
            button.imagePosition = NSImageOnly;
        } else if (@available(macOS 11.0, *)) {
            NSImage* fallback = [NSImage imageWithSystemSymbolName:@"line.3.horizontal.decrease.circle"
                                         accessibilityDescription:@"Log Sift"];
            fallback.template = YES;
            button.image = fallback;
            button.imagePosition = NSImageOnly;
        } else {
            button.title = @"LS";
        }

        NSMenu* menu = [NSMenu new];

        NSMenuItem* open = [[NSMenuItem alloc] initWithTitle:@"Open Log Sift"
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

        NSMenuItem* copy = [[NSMenuItem alloc] initWithTitle:@"Copy Results"
                                                     action:@selector(copyResults:)
                                              keyEquivalent:@""];
        copy.target = gTarget;
        [menu addItem:copy];

        [menu addItem:[NSMenuItem separatorItem]];

        NSMenuItem* quit = [[NSMenuItem alloc] initWithTitle:@"Quit Log Sift"
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
