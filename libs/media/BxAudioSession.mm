#include "BxAudioSession.h"

#import <AVFAudio/AVFAudio.h>
#import <UIKit/UIKit.h>

#include <algorithm>
#include <mutex>
#include <vector>

static std::mutex bxAudioSessionMutex;
static std::vector<AudioComponentInstance> bxAudioSessionUnits;
static bool bxAudioSessionObserving = false;
static bool bxAudioSessionInBackground = false;

static void bxAudioSessionResume() {
    std::lock_guard<std::mutex> lock(bxAudioSessionMutex);
    if (bxAudioSessionInBackground) {
        return;
    }
    [[AVAudioSession sharedInstance] setActive:YES error:nil];
    for (AudioComponentInstance unit : bxAudioSessionUnits) {
        AudioOutputUnitStart(unit);
    }
}

static void bxAudioSessionEnterBackground() {
    std::lock_guard<std::mutex> lock(bxAudioSessionMutex);
    bxAudioSessionInBackground = true;
    for (AudioComponentInstance unit : bxAudioSessionUnits) {
        AudioOutputUnitStop(unit);
    }
}

static void bxAudioSessionEnterForeground() {
    {
        std::lock_guard<std::mutex> lock(bxAudioSessionMutex);
        bxAudioSessionInBackground = false;
    }
    bxAudioSessionResume();
}

static void bxAudioSessionObserve(NSString* name, id object, void (^handler)(NSNotification*)) {
    [[[NSNotificationCenter defaultCenter] addObserverForName:name
                                                       object:object
                                                        queue:nil
                                                   usingBlock:handler] retain];
}

bool bxAudioSessionActivate(std::string& error) {
    @autoreleasepool {
        AVAudioSession* session = [AVAudioSession sharedInstance];
        NSError* failure = nil;
        if (![session setCategory:AVAudioSessionCategoryAmbient error:&failure] ||
            ![session setActive:YES error:&failure]) {
            error = std::string("Activating the audio session failed: ") +
                    failure.localizedDescription.UTF8String;
            return false;
        }
        if (!bxAudioSessionObserving) {
            bxAudioSessionObserving = true;
            bxAudioSessionObserve(AVAudioSessionInterruptionNotification, session, ^(NSNotification* notification) {
                NSNumber* type = notification.userInfo[AVAudioSessionInterruptionTypeKey];
                if (type.unsignedIntegerValue == AVAudioSessionInterruptionTypeEnded) {
                    bxAudioSessionResume();
                }
            });
            bxAudioSessionObserve(UIApplicationDidEnterBackgroundNotification, nil, ^(NSNotification*) {
                bxAudioSessionEnterBackground();
            });
            bxAudioSessionObserve(UIApplicationWillEnterForegroundNotification, nil, ^(NSNotification*) {
                bxAudioSessionEnterForeground();
            });
        }
        return true;
    }
}

void bxAudioSessionAttach(AudioComponentInstance unit) {
    std::lock_guard<std::mutex> lock(bxAudioSessionMutex);
    bxAudioSessionUnits.push_back(unit);
}

void bxAudioSessionDetach(AudioComponentInstance unit) {
    std::lock_guard<std::mutex> lock(bxAudioSessionMutex);
    bxAudioSessionUnits.erase(
        std::remove(bxAudioSessionUnits.begin(), bxAudioSessionUnits.end(), unit),
        bxAudioSessionUnits.end());
}
