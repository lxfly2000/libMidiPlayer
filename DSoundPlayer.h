#pragma once
#include "WavePlayerBase.h"
class DSoundPlayer :public WavePlayerBase
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
};
