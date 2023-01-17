#pragma once
#include <Windows.h>
class WavePlayerBase
{
public:
	virtual void GetClass(char *, int) = 0;
	//bytesPerVar:ָ����һ���������һ��ͨ��ռ�ֽ���
	virtual int Init(int nChannel, int sampleRate, int bytesPerVar) = 0;
	virtual void Release() = 0;
	virtual void Play(BYTE* buf, int length, int smpPerCh, int channels, float**sndData) = 0;
	//ȡֵ��ΧΪ[0,1]
	virtual void SetVolume(float v) = 0;
	//ȡֵ��ΧΪ[0,1]
	virtual float GetVolume() = 0;
	//1Ϊԭ��
	virtual int SetPlaybackSpeed(float) = 0;
	virtual int GetQueuedBuffersNum() = 0;
	virtual void WaitForBufferEndEvent() = 0;
	virtual int GetChannelCount() = 0;
	virtual int GetSampleRate() = 0;
	//ָ����һ���������һ��ͨ��ռ�ֽ���
	virtual int GetBytesPerVar() = 0;
	virtual int GetBufferTimeMS() = 0;
};
