#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include "dialog_macos.h"

static bool showConfirm(const char* title, const char* message) {
    @autoreleasepool {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.alertStyle = NSAlertStyleWarning;
        alert.messageText = title ? [NSString stringWithUTF8String:title] : @"Confirm";
        alert.informativeText = message ? [NSString stringWithUTF8String:message] : @"";
        [alert addButtonWithTitle:@"OK"];
        [alert addButtonWithTitle:@"Cancel"];
        [alert setIcon:[NSImage imageNamed:NSImageNameCaution]];

        // Bring to front so the prompt is not missed while the GLFW window is focused.
        [NSApp activateIgnoringOtherApps:YES];
        NSModalResponse response = [alert runModal];
        return response == NSAlertFirstButtonReturn;
    }
}

bool macConfirmDialog(const char* title, const char* message) {
    // AppKit modal loops must run on the main thread. All current callers are on
    // the UI thread, but hop over if one ever comes from a worker.
    if ([NSThread isMainThread]) {
        return showConfirm(title, message);
    }
    __block bool accepted = false;
    dispatch_sync(dispatch_get_main_queue(), ^{
        accepted = showConfirm(title, message);
    });
    return accepted;
}
