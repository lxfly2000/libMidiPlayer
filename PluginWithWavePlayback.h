#pragma once
#include "WavePlayerBase.h"
#include <thread>
class PluginWithWavePlayback
{
public:
	PluginWithWavePlayback();
	//���ز���ļ�����Դ�ļ����ɹ�����0�����򷵻�-1
	virtual int LoadPlugin(LPCTSTR path, int smpRate = 44100) = 0;
	virtual int ReleasePlugin() = 0;
	int InitPlayer(int nChannel = 2, int smpRate = 44100);
	int ReleasePlayer();
	//ȡֵ��Χ��[0,1]
	void SetVolume(float);
	//ȡֵ��Χ��[0,1]
	float GetVolume(float);
	int StartPlayback();
	int StopPlayback();
	static void _Subthread_Playback(PluginWithWavePlayback*);
	virtual void _Playback() = 0;
	virtual int SendMidiData(DWORD midiData) = 0;
	virtual int SendSysExData(LPVOID data,DWORD length) = 0;
	virtual void CommitSend() = 0;
	//����Wav���ɹ�����0��ʧ�ܷ���-1����֧�ַ���-2
	virtual int ExportToWav(LPCTSTR midiFilePath, LPCTSTR wavFilePath, LPVOID extraInfo = NULL);
protected:
	static WavePlayerBase *player;
	int isPlayingBack;//0��ʾֹͣ��1��ʾ���ڲ�����Ƶ��
	std::thread threadPlayback;
};
