#include "YuAudioSession.h"

#import <AVFAudio/AVFAudio.h>
#import <UIKit/UIKit.h>

#include <algorithm>
#include <mutex>
#include <vector>

static std::mutex yuAudioSessionMutex;
static std::vector<AudioComponentInstance> yuAudioSessionUnits;
static bool yuAudioSessionObserving = false;
static bool yuAudioSessionInBackground = false;

static void yuAudioSessionResume() {
    std::lock_guard<std::mutex> lock(yuAudioSessionMutex);
    if (yuAudioSessionInBackground) {
        return;
    }
    [[AVAudioSession sharedInstance] setActive:YES error:nil];
    for (AudioComponentInstance unit : yuAudioSessionUnits) {
        AudioOutputUnitStart(unit);
    }
}

static void yuAudioSessionEnterBackground() {
    std::lock_guard<std::mutex> lock(yuAudioSessionMutex);
    yuAudioSessionInBackground = true;
    for (AudioComponentInstance unit : yuAudioSessionUnits) {
        AudioOutputUnitStop(unit);
    }
}

static void yuAudioSessionEnterForeground() {
    {
        std::lock_guard<std::mutex> lock(yuAudioSessionMutex);
        yuAudioSessionInBackground = false;
    }
    yuAudioSessionResume();
}

static void yuAudioSessionObserve(NSString* name, id object, void (^handler)(NSNotification*)) {
    [[[NSNotificationCenter defaultCenter] addObserverForName:name
                                                       object:object
                                                        queue:nil
                                                   usingBlock:handler] retain];
}

bool yuAudioSessionActivate(std::string& error) {
    @autoreleasepool {
        AVAudioSession* session = [AVAudioSession sharedInstance];
        NSError* failure = nil;
        if (![session setCategory:AVAudioSessionCategoryAmbient error:&failure] ||
            ![session setActive:YES error:&failure]) {
            error = std::string("Activating the audio session failed: ") +
                    failure.localizedDescription.UTF8String;
            return false;
        }
        if (!yuAudioSessionObserving) {
            yuAudioSessionObserving = true;
            yuAudioSessionObserve(AVAudioSessionInterruptionNotification, session, ^(NSNotification* notification) {
                NSNumber* type = notification.userInfo[AVAudioSessionInterruptionTypeKey];
                if (type.unsignedIntegerValue == AVAudioSessionInterruptionTypeEnded) {
                    yuAudioSessionResume();
                }
            });
            yuAudioSessionObserve(UIApplicationDidEnterBackgroundNotification, nil, ^(NSNotification*) {
                yuAudioSessionEnterBackground();
            });
            yuAudioSessionObserve(UIApplicationWillEnterForegroundNotification, nil, ^(NSNotification*) {
                yuAudioSessionEnterForeground();
            });
        }
        return true;
    }
}

void yuAudioSessionAttach(AudioComponentInstance unit) {
    std::lock_guard<std::mutex> lock(yuAudioSessionMutex);
    yuAudioSessionUnits.push_back(unit);
}

void yuAudioSessionDetach(AudioComponentInstance unit) {
    std::lock_guard<std::mutex> lock(yuAudioSessionMutex);
    yuAudioSessionUnits.erase(
        std::remove(yuAudioSessionUnits.begin(), yuAudioSessionUnits.end(), unit),
        yuAudioSessionUnits.end());
}
