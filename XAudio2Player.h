#pragma once

#include "WavePlayerBase.h"
#include <xaudio2.h>

class XASCallback :public IXAudio2VoiceCallback
{
public:
	HANDLE hBufferEndEvent;
	XASCallback();
	~XASCallback();
	void WINAPI OnBufferEnd(void*)override;
	void WINAPI OnBufferStart(void*)override;
	void WINAPI OnLoopEnd(void*)override;
	void WINAPI OnStreamEnd()override;
	void WINAPI OnVoiceError(void*, HRESULT)override;
	void WINAPI OnVoiceProcessingPassEnd()override;
	void WINAPI OnVoiceProcessingPassStart(UINT32)override;
};


class XAudio2Player:public WavePlayerBase
{
public:
	virtual void GetClass(char *, int);
	//bytesPerVar:ָ����һ���������һ��ͨ��ռ�ֽ���
	virtual int Init(int nChannel, int sampleRate, int bytesPerVar);
	virtual void Release();
	virtual void Play(BYTE* buf, int length);
	//ȡֵ��ΧΪ[0,1]
	virtual void SetVolume(float v);
	//ȡֵ��ΧΪ[0,1]
	virtual float GetVolume();
	//1Ϊԭ��
	virtual int SetPlaybackSpeed(float);
	virtual int GetQueuedBuffersNum();
	virtual void WaitForBufferEndEvent();
	virtual int GetChannelCount();
	virtual int GetSampleRate();
	//ָ����һ���������һ��ͨ��ռ�ֽ���
	virtual int GetBytesPerVar();
	virtual int GetBufferTimeMS();
private:
	IXAudio2* xAudio2Engine;
	IXAudio2MasteringVoice* masterVoice;
	IXAudio2SourceVoice* sourceVoice;
	XAUDIO2_BUFFER xbuffer;
	XAUDIO2_VOICE_STATE state;
	XASCallback xcallback;
	int m_channels, m_sampleRate, m_bytesPerVar;
};
