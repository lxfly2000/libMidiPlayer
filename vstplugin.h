#pragma once
#include "PluginWithXAudio2Playback.h"
class VstPlugin:public PluginWithXAudio2Playback
{
public:
	int LoadPlugin(LPCTSTR path, int smpRate = 44100)override;
	int ReleasePlugin()override;
	int ShowPluginWindow(bool);
	bool IsPluginWindowShown();
	void _Playback()override;
	int SendMidiData(DWORD midiData)override;
	int SendSysExData(LPVOID data, DWORD length)override;
private:
	bool isPluginWindowShown;
};
