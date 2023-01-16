#pragma once
#include "WavePlayerBase.h"
class DSoundPlayer :public WavePlayerBase
{
public:
	virtual void GetClass(char *, int);
	//bytesPerVar:指的是一个采样点的一个通道占字节数
	virtual int Init(int nChannel, int sampleRate, int bytesPerVar);
	virtual void Release();
	virtual void Play(BYTE* buf, int length);
	//取值范围为[0,1]
	virtual void SetVolume(float v);
	//取值范围为[0,1]
	virtual float GetVolume();
	//1为原速
	virtual int SetPlaybackSpeed(float);
	virtual int GetQueuedBuffersNum();
	virtual void WaitForBufferEndEvent();
	virtual int GetChannelCount();
	virtual int GetSampleRate();
	//指的是一个采样点的一个通道占字节数
	virtual int GetBytesPerVar();
	virtual int GetBufferTimeMS();
};
