#import <Cocoa/Cocoa.h>

static bool gOpen=false, gToggleWatch=false, gCopy=false, gQuit=false;
static bool gWatch=true;
static NSStatusItem* gItem=nil;

@interface LogSiftStatusTarget : NSObject
- (void)openApp:(id)sender;
- (void)toggleWatch:(id)sender;
- (void)copyResults:(id)sender;
- (void)quitApp:(id)sender;
@end

@implementation LogSiftStatusTarget
- (void)openApp:(id)sender { (void)sender; gOpen=true; }
- (void)toggleWatch:(id)sender { (void)sender; gToggleWatch=true; }
- (void)copyResults:(id)sender { (void)sender; gCopy=true; }
- (void)quitApp:(id)sender { (void)sender; gQuit=true; }
@end

static LogSiftStatusTarget* gTarget=nil;

extern "C" void LogSiftMacTrayInit(void) {
    @autoreleasepool {
        if (gItem) return;
        gTarget=[LogSiftStatusTarget new];
        gItem=[[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
        gItem.button.title=@"LS";
        gItem.button.toolTip=@"Log Sift";
        NSMenu* menu=[NSMenu new];
        NSMenuItem* open=[[NSMenuItem alloc] initWithTitle:@"Open Log Sift" action:@selector(openApp:) keyEquivalent:@""];
        NSMenuItem* watch=[[NSMenuItem alloc] initWithTitle:@"Watch Clipboard" action:@selector(toggleWatch:) keyEquivalent:@""];
        watch.state=NSControlStateValueOn;
        watch.tag=42;
        NSMenuItem* copy=[[NSMenuItem alloc] initWithTitle:@"Copy Results" action:@selector(copyResults:) keyEquivalent:@""];
        NSMenuItem* quit=[[NSMenuItem alloc] initWithTitle:@"Quit Log Sift" action:@selector(quitApp:) keyEquivalent:@""];
        for(NSMenuItem* i in @[open,watch,copy,quit]) i.target=gTarget;
        [menu addItem:open]; [menu addItem:watch]; [menu addItem:copy];
        [menu addItem:[NSMenuItem separatorItem]]; [menu addItem:quit];
        gItem.menu=menu;
        [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
    }
}
extern "C" bool LogSiftMacTrayTakeOpen(void){ bool v=gOpen; gOpen=false; return v; }
extern "C" bool LogSiftMacTrayTakeToggleWatch(void){ bool v=gToggleWatch; gToggleWatch=false; return v; }
extern "C" bool LogSiftMacTrayTakeCopy(void){ bool v=gCopy; gCopy=false; return v; }
extern "C" bool LogSiftMacTrayTakeQuit(void){ bool v=gQuit; gQuit=false; return v; }
extern "C" void LogSiftMacTraySetWatch(bool enabled){
    gWatch=enabled;
    if(!gItem) return;
    NSMenuItem* item=[gItem.menu itemWithTag:42];
    item.state=enabled?NSControlStateValueOn:NSControlStateValueOff;
}
