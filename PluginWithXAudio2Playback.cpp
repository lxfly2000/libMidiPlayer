#include "PluginWithXAudio2Playback.h"

XAudio2Player PluginWithXAudio2Playback::player;

PluginWithXAudio2Playback::PluginWithXAudio2Playback():isPlayingBack(0)
{
}

int PluginWithXAudio2Playback::InitPlayer(int nChannel, int smpRate)
{
	return player.Init(nChannel, smpRate, 2);
}

int PluginWithXAudio2Playback::ReleasePlayer()
{
	player.Release();
	return 0;
}

void PluginWithXAudio2Playback::SetVolume(float v)
{
	player.SetVolume(v);
}

float PluginWithXAudio2Playback::GetVolume(float)
{
	return player.GetVolume();
}

int PluginWithXAudio2Playback::StartPlayback()
{
	if (isPlayingBack)
		return -1;
	isPlayingBack = 1;
	threadPlayback = std::thread(_Subthread_Playback, this);
	return 0;
}

int PluginWithXAudio2Playback::StopPlayback()
{
	if (isPlayingBack == 0)
		return -1;
	isPlayingBack = 0;
	threadPlayback.join();
	return 0;
}

void PluginWithXAudio2Playback::_Subthread_Playback(PluginWithXAudio2Playback*p)
{
	p->_Playback();
}

int PluginWithXAudio2Playback::ExportToWav(LPCTSTR midiFilePath, LPCTSTR wavFilePath)
{
	return -2;
}
