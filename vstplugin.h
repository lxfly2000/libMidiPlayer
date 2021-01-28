#pragma once
#include "PluginWithXAudio2Playback.h"
#include <vector>
class VstPlugin :public PluginWithXAudio2Playback
{
public:
	int LoadPlugin(LPCTSTR path, int smpRate = 44100)override;
	int ReleasePlugin()override;
	int ShowPluginWindow(bool);
	bool IsPluginWindowShown();
	void _Playback()override;
	int SendMidiData(DWORD midiData)override;
	int SendSysExData(LPVOID data, DWORD length)override;
	void CommitSend();
	void OnIdle();
	int ExportToWav(LPCTSTR midiFilePath, LPCTSTR wavFilePath, LPVOID extraInfo = NULL)override;
private:
	int m_smpRate, m_blockSize;
	UINT buffer_time_ms;
	size_t bytesof_soundBuffer, singleChSamples;
	short* buffer;
	std::vector<float*>pfChannelsIn, pfChannelsOut;
	long numInputs, numOutputs;
};
