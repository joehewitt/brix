#pragma once

#include <AudioToolbox/AudioToolbox.h>

#include <string>

bool bxAudioSessionActivate(std::string& error);
void bxAudioSessionAttach(AudioComponentInstance unit);
void bxAudioSessionDetach(AudioComponentInstance unit);
