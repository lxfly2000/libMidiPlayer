#include "XAudio2Player.h"


XASCallback::XASCallback()
{
	hBufferEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

XASCallback::~XASCallback()
{
	CloseHandle(hBufferEndEvent);
}

void WINAPI XASCallback::OnBufferEnd(void* p)
{
	SetEvent(hBufferEndEvent);
}

void WINAPI XASCallback::OnBufferStart(void*)
{
	//Nothing
}

void WINAPI XASCallback::OnLoopEnd(void*)
{
	//Nothing
}

void WINAPI XASCallback::OnStreamEnd()
{
	//Nothing
}

void WINAPI XASCallback::OnVoiceError(void*, HRESULT)
{
	//Nothing
}

void WINAPI XASCallback::OnVoiceProcessingPassEnd()
{
	//Nothing
}

void WINAPI XASCallback::OnVoiceProcessingPassStart(UINT32)
{
	//Nothing
}


int XAudio2Player::Init(int nChannel, int sampleRate, int bytesPerVar)
{
	m_channels = nChannel;
	m_sampleRate = sampleRate;
	m_bytesPerVar = bytesPerVar;
	xAudio2Engine = NULL;
	masterVoice = NULL;
	sourceVoice = NULL;
	PCMWAVEFORMAT pw{};
	pw.wBitsPerSample = bytesPerVar * 8;
	WAVEFORMAT& w = pw.wf;
	w.wFormatTag = WAVE_FORMAT_PCM;
	w.nChannels = nChannel;
	w.nSamplesPerSec = sampleRate;
	w.nAvgBytesPerSec = sampleRate * bytesPerVar * nChannel;
	w.nBlockAlign = bytesPerVar * nChannel;
	xbuffer = {};
	xbuffer.Flags = XAUDIO2_END_OF_STREAM;
	/*if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED)))
		return -1;*/
	if (FAILED(XAudio2Create(&xAudio2Engine)))
		return -2;
	if (FAILED(xAudio2Engine->CreateMasteringVoice(&masterVoice)))
		return -3;
	if (FAILED(xAudio2Engine->CreateSourceVoice(&sourceVoice, (WAVEFORMATEX*)&pw, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &xcallback)))
		return -4;
	if (FAILED(sourceVoice->Start()))
		return -5;
	return 0;
}

void XAudio2Player::Release()
{
	if (sourceVoice)
	{
		sourceVoice->Stop();
		sourceVoice->FlushSourceBuffers();
		sourceVoice->DestroyVoice();
	}
	if (masterVoice)
		masterVoice->DestroyVoice();
	if (xAudio2Engine)
		xAudio2Engine->Release();
	//CoUninitialize();
}

void XAudio2Player::Play(BYTE* buf, int length)
{
	xbuffer.pAudioData = buf;
	xbuffer.AudioBytes = length;
	sourceVoice->SubmitSourceBuffer(&xbuffer);
}

void XAudio2Player::SetVolume(float v)
{
	masterVoice->SetVolume(v);
}

float XAudio2Player::GetVolume()
{
	float v;
	masterVoice->GetVolume(&v);
	return v;
}

int XAudio2Player::GetQueuedBuffersNum()
{
	sourceVoice->GetState(&state);
	return state.BuffersQueued;
}

void XAudio2Player::WaitForBufferEndEvent()
{
	WaitForSingleObject(xcallback.hBufferEndEvent, INFINITE);
}

int XAudio2Player::GetChannelCount()
{
	return m_channels;
}

int XAudio2Player::GetSampleRate()
{
	return m_sampleRate;
}

int XAudio2Player::GetBytesPerVar()
{
	return m_bytesPerVar;
}

int XAudio2Player::SetPlaybackSpeed(float speed)
{
	return sourceVoice->SetFrequencyRatio(speed);
}
