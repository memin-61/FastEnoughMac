#import <Foundation/Foundation.h>
#include "trash_macos.h"
#include <string>
#include <vector>

bool trashFilesMac(const std::vector<std::string>& paths, std::string& error) {
    @autoreleasepool {
        NSFileManager *fm = [NSFileManager defaultManager];
        for (const auto& p : paths) {
            NSString *nsPath = [NSString stringWithUTF8String:p.c_str()];
            if (!nsPath) {
                error = "Invalid path encoding: " + p;
                return false;
            }
            NSURL *url = [NSURL fileURLWithPath:nsPath];
            NSError *err = nil;
            NSURL *resultingURL = nil;
            // trashItemAtURL is available on 10.11+
            BOOL ok = [fm trashItemAtURL:url resultingItemURL:&resultingURL error:&err];
            if (!ok) {
                if (err) {
                    NSString *desc = [err localizedDescription];
                    if (desc) error = [desc UTF8String];
                    else error = "Failed to move to Trash";
                } else {
                    error = "Failed to move to Trash: " + p;
                }
                return false;
            }
        }
    }
    return true;
}
