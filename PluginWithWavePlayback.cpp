#include "PluginWithWavePlayback.h"
#include "XAudio2Player.h"
#include "DSoundPlayer.h"

WavePlayerBase* PluginWithWavePlayback::player = nullptr;

PluginWithWavePlayback::PluginWithWavePlayback():isPlayingBack(0)
{
}

int PluginWithWavePlayback::InitPlayer(int nChannel, int smpRate)
{
	player = new XAudio2Player;
	if (player->Init(nChannel, smpRate, 2) == 0)
		return 0;
	delete player;
	player = new DSoundPlayer;
	return player->Init(nChannel, smpRate, 2);
}

int PluginWithWavePlayback::ReleasePlayer()
{
	player->Release();
	delete player;
	player = nullptr;
	return 0;
}

void PluginWithWavePlayback::SetVolume(float v)
{
	player->SetVolume(v);
}

float PluginWithWavePlayback::GetVolume(float)
{
	return player->GetVolume();
}

int PluginWithWavePlayback::StartPlayback()
{
	if (isPlayingBack)
		return -1;
	isPlayingBack = 1;
	threadPlayback = std::thread(_Subthread_Playback, this);
	return 0;
}

int PluginWithWavePlayback::StopPlayback()
{
	if (isPlayingBack == 0)
		return -1;
	isPlayingBack = 0;
	threadPlayback.join();
	return 0;
}

void PluginWithWavePlayback::_Subthread_Playback(PluginWithWavePlayback*p)
{
	p->_Playback();
}

int PluginWithWavePlayback::ExportToWav(LPCTSTR midiFilePath, LPCTSTR wavFilePath, LPVOID extraInfo)
{
	return -2;
}
