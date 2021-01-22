#pragma once

#include "XAudio2_8/Include/um/xaudio2.h"

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


class XAudio2Player
{
public:
	int Init(int nChannel, int sampleRate, int bytesPerVar);
	void Release();
	void Play(BYTE* buf, int length);
	//取值范围为[0,1]
	void SetVolume(float v);
	//取值范围为[0,1]
	float GetVolume();
	//1为原速
	int SetPlaybackSpeed(float);
	int GetQueuedBuffersNum();
	void WaitForBufferEndEvent();
	int GetChannelCount();
	int GetSampleRate();
	int GetBytesPerVar();
private:
	IXAudio2* xAudio2Engine;
	IXAudio2MasteringVoice* masterVoice;
	IXAudio2SourceVoice* sourceVoice;
	XAUDIO2_BUFFER xbuffer;
	XAUDIO2_VOICE_STATE state;
	XASCallback xcallback;
	int m_channels, m_sampleRate, m_bytesPerVar;
};
