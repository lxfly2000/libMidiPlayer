#include "DSoundPlayer.h"
#include "common.h"
#include <dsound.h>

#pragma comment(lib,"dsound.lib")
#pragma comment(lib,"dxguid.lib")

#define SECTION_NAME "DSound"
#define KEYNAME_NOTIFY_COUNT "DSoundNotifyCount"
#define KEYNAME_BUFFER_LENGTH "DSoundBufferTime"
#define VDEFAULT_NOTIFY_COUNT 4
#define VDEFAULT_BUFFER_LENGTH 50

#ifdef _DEBUG
#define C(e) if(e)\
if(MessageBox(NULL,__FILE__ L":" _CRT_STRINGIZE(__LINE__) "\n" __FUNCTION__ "\n" _CRT_STRINGIZE(e),NULL,MB_OKCANCEL)==IDOK)\
DebugBreak()
#else
#define C(e) e
#endif

DWORD m_bufbytes=0, writecursor = 0;
DWORD lockedBufferBytes=0;
void *pLockedBuffer=nullptr;
HANDLE hBufferEndEvent=NULL;
IDirectSound8 *pDirectSound=nullptr;
IDirectSoundBuffer *pBuffer=nullptr;
IDirectSoundNotify *pNotify=nullptr;
WAVEFORMATEX w{};

void DSoundPlayer::GetClass(char *buf, int n)
{
	lstrcpynA(buf, "DSound", n);
}

int DSoundPlayer::Init(int nChannel, int sampleRate, int bytesPerVar)
{
	w.wFormatTag = WAVE_FORMAT_PCM;
	w.nChannels = nChannel;
	w.nSamplesPerSec = sampleRate;
	w.nAvgBytesPerSec = sampleRate * bytesPerVar*nChannel;
	w.nBlockAlign = 4;
	w.wBitsPerSample = bytesPerVar * 8;
	w.cbSize = 0;

	int notify_count = GetPrivateProfileInt(TEXT(SECTION_NAME), TEXT(KEYNAME_NOTIFY_COUNT), VDEFAULT_NOTIFY_COUNT, GetProfilePath());
	int buffer_time_ms = GetBufferTimeMS();
	int bytesof_soundbuffer = sampleRate * bytesPerVar*nChannel*buffer_time_ms / 1000;//onebufbyte
	m_bufbytes = bytesof_soundbuffer * notify_count;

	C(DirectSoundCreate8(NULL, &pDirectSound, NULL));
	C(pDirectSound->SetCooperativeLevel(GetDesktopWindow(), DSSCL_NORMAL));
	DSBUFFERDESC desc = { sizeof desc,DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPOSITIONNOTIFY |
		DSBCAPS_GLOBALFOCUS,m_bufbytes,0,&w,DS3DALG_DEFAULT };
	C(pDirectSound->CreateSoundBuffer(&desc, &pBuffer, NULL));
	C(pBuffer->QueryInterface(IID_IDirectSoundNotify, (void**)&pNotify));
	DSBPOSITIONNOTIFY *dpn = new DSBPOSITIONNOTIFY[notify_count];
	hBufferEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	for (int i = 0; i < notify_count; i++)
	{
		dpn[i].dwOffset = m_bufbytes * i / notify_count;
		dpn[i].hEventNotify = hBufferEndEvent;
	}
	C(pNotify->SetNotificationPositions(notify_count, dpn));
	delete[]dpn;
	C(pBuffer->Play(0, 0, DSBPLAY_LOOPING));
	return 0;
}

void DSoundPlayer::Release()
{
	C(pBuffer->Stop());
	CloseHandle(hBufferEndEvent);
	pNotify->Release();
	pBuffer->Release();
	pDirectSound->Release();
	hBufferEndEvent = NULL;
	pNotify = nullptr;
	pBuffer = nullptr;
	pDirectSound = nullptr;
}

void DSoundPlayer::Play(BYTE* buf, int length, int smpPerCh, int channels, float**sndData)
{
	C(pBuffer->Lock(writecursor, length, &pLockedBuffer, &lockedBufferBytes, NULL, NULL, NULL));
	for (int i = 0; i < smpPerCh; i++)
	{
		for (int j = 0; j < channels; j++)
		{
			//某些做得比较糙的插件会出现数值在[-1,1]范围以外的情况，注意限定范围
			((short*)pLockedBuffer)[channels * i + j] = (short)(min(max(-32768.f, sndData[j][i] * 32767.f), 32767.f));
		}
	}
	C(pBuffer->Unlock(pLockedBuffer, lockedBufferBytes, NULL, NULL));
	writecursor = (writecursor + lockedBufferBytes) % m_bufbytes;
	WaitForBufferEndEvent();
}

void DSoundPlayer::SetVolume(float v)
{
	C(pBuffer->SetVolume(v*(DSBVOLUME_MAX - DSBVOLUME_MIN) - (DSBVOLUME_MAX - DSBVOLUME_MIN)));
}

float DSoundPlayer::GetVolume()
{
	long v;
	C(pBuffer->GetVolume(&v));
	return (float)(v - DSBVOLUME_MIN) / (DSBVOLUME_MAX - DSBVOLUME_MIN);
}

int DSoundPlayer::SetPlaybackSpeed(float speed)
{
	if (FAILED(pBuffer->SetFrequency((DWORD)(w.nSamplesPerSec*speed))))
		return -1;
	return 0;
}

int DSoundPlayer::GetQueuedBuffersNum()
{
	//TODO
	return 0;
}

void DSoundPlayer::WaitForBufferEndEvent()
{
	WaitForSingleObject(hBufferEndEvent, INFINITE);
}

int DSoundPlayer::GetChannelCount()
{
	return w.nChannels;
}

int DSoundPlayer::GetSampleRate()
{
	return w.nSamplesPerSec;
}

int DSoundPlayer::GetBytesPerVar()
{
	return w.wBitsPerSample / 8;
}

int DSoundPlayer::GetBufferTimeMS()
{
	return GetPrivateProfileInt(TEXT(SECTION_NAME), TEXT(KEYNAME_BUFFER_LENGTH), VDEFAULT_BUFFER_LENGTH, GetProfilePath());
}
