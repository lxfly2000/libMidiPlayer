#pragma once
#include "WavePlayerBase.h"
#include <thread>
class PluginWithWavePlayback
{
public:
	PluginWithWavePlayback();
	//加载插件文件或音源文件，成功返回0，否则返回-1
	virtual int LoadPlugin(LPCTSTR path, int smpRate = 44100) = 0;
	virtual int ReleasePlugin() = 0;
	int InitPlayer(int nChannel = 2, int smpRate = 44100);
	int ReleasePlayer();
	//取值范围：[0,1]
	void SetVolume(float);
	//取值范围：[0,1]
	float GetVolume(float);
	int StartPlayback();
	int StopPlayback();
	static void _Subthread_Playback(PluginWithWavePlayback*);
	virtual void _Playback() = 0;
	virtual int SendMidiData(DWORD midiData) = 0;
	virtual int SendSysExData(LPVOID data,DWORD length) = 0;
	virtual void CommitSend() = 0;
	//导出Wav，成功返回0，失败返回-1，不支持返回-2
	virtual int ExportToWav(LPCTSTR midiFilePath, LPCTSTR wavFilePath, LPVOID extraInfo = NULL);
protected:
	static WavePlayerBase *player;
	int isPlayingBack;//0表示停止，1表示正在播放音频流
	std::thread threadPlayback;
};
