#pragma once
#include <Windows.h>
class WavePlayerBase
{
public:
	virtual void GetClass(char *, int) = 0;
	//bytesPerVar:指的是一个采样点的一个通道占字节数
	virtual int Init(int nChannel, int sampleRate, int bytesPerVar) = 0;
	virtual void Release() = 0;
	virtual void Play(BYTE* buf, int length, int smpPerCh, int channels, float**sndData) = 0;
	//取值范围为[0,1]
	virtual void SetVolume(float v) = 0;
	//取值范围为[0,1]
	virtual float GetVolume() = 0;
	//1为原速
	virtual int SetPlaybackSpeed(float) = 0;
	virtual int GetQueuedBuffersNum() = 0;
	virtual void WaitForBufferEndEvent() = 0;
	virtual int GetChannelCount() = 0;
	virtual int GetSampleRate() = 0;
	//指的是一个采样点的一个通道占字节数
	virtual int GetBytesPerVar() = 0;
	virtual int GetBufferTimeMS() = 0;
};
