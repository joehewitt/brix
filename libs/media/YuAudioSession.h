#pragma once

#include <AudioToolbox/AudioToolbox.h>

#include <string>

bool yuAudioSessionActivate(std::string& error);
void yuAudioSessionAttach(AudioComponentInstance unit);
void yuAudioSessionDetach(AudioComponentInstance unit);
