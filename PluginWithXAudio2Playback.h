#pragma once
#include "XAudio2Player.h"
#include <thread>
class PluginWithXAudio2Playback
{
public:
	//���ز���ļ�����Դ�ļ����ɹ�����0�����򷵻�-1
	virtual int LoadPlugin(LPCTSTR path, int smpRate = 44100) = 0;
	virtual int ReleasePlugin() = 0;
	//ȡֵ��Χ��[0,1]
	void SetVolume(float);
	//ȡֵ��Χ��[0,1]
	float GetVolume(float);
	int StartPlayback(int nChannel = 2, int smpRate = 44100);
	int StopPlayback();
	static void _Subthread_Playback(PluginWithXAudio2Playback*);
	virtual void _Playback() = 0;
	virtual int SendMidiData(DWORD midiData) = 0;
	virtual int SendSysExData(LPVOID data,DWORD length) = 0;
protected:
	XAudio2Player player;
	int isPlayingBack;//0��ʾֹͣ��1��ʾ���ڲ�����Ƶ��
	std::thread threadPlayback;
};
