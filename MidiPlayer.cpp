#include "MidiPlayer.h"
#include <fstream>
#include <sstream>

MidiPlayer *pmp = nullptr;
void WINAPI OnTimerFunc(UINT wTimerID, UINT msg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	pmp->_TimerFunc(wTimerID, msg, dwUser, dw1, dw2);
}

MidiPlayer::MidiPlayer() :volume(MIDIPLAYER_MAX_VOLUME), sendLongMsg(true), timerID(0), deltaTime(10),
midiSysExMsg(nullptr), nMaxSysExMsg(256), nChannels(16), nKeys(128)
{
	if (pmp)delete pmp;
	pmp = this;
	midiSysExMsg = new BYTE[nMaxSysExMsg]{ 0 };
	keyPressure = new unsigned char[nChannels*nKeys];
	VarReset();
	ZeroMemory(&header, sizeof(header));
	header.lpData = (LPSTR)midiSysExMsg;
	header.dwBufferLength = nMaxSysExMsg;
	header.dwFlags = 0;
	midiOutOpen(&hMidiOut, MIDI_MAPPER, 0, 0, 0);
}

MidiPlayer::~MidiPlayer()
{
	Stop();
	Unload();
	midiOutClose(hMidiOut);
	delete[]keyPressure;
	delete[]midiSysExMsg;
	pmp = nullptr;
}

void MidiPlayer::VarReset()
{
	loopStartTick = loopEndTick = 0.0f;
	tempo = 120.0f;
	nextTick = 0.0f;
	nEvent = 0;
	nEventCount = 0;
	nMsgSize = 0;
	midiEvent = 0;
	nLoopStartEvent = 0;
	nPlayStatus = 0;
	polyphone = 0;
}

void MidiPlayer::SetKeyPressure(unsigned channel, unsigned key, unsigned char pressure)
{
	if (channel < nChannels&&key < nKeys)
		keyPressure[nKeys*channel + key] = pressure;
}

bool MidiPlayer::LoadFile(const char *filename)
{
	//http://blog.csdn.net/tulip527/article/details/7976471
	std::ifstream file(filename, std::ios::in | std::ios::binary);
	std::stringstream filemem;
	if (!file)return false;
	filemem << file.rdbuf();
	return LoadStream(filemem);
}

bool MidiPlayer::LoadStream(std::stringstream &mem)
{
	Stop();
	Unload();
	//read使用的是不带const的变量
	if (!midifile.read(mem))return false;
	VarReset();
	midifile.joinTracks();
	nEventCount = midifile[0].size();
	return true;
}

void MidiPlayer::Unload()
{
	midifile.clear();
}

bool MidiPlayer::Play(bool goOn)
{
	if (nPlayStatus == 1)return true;
	if (!midifile[0].size())return false;
	if (!goOn)Stop();
	timerID = timeSetEvent(deltaTime, 1, OnTimerFunc, 1, TIME_PERIODIC);
	nPlayStatus = 1;
	return true;
}

void MidiPlayer::Pause()
{
	if (nPlayStatus == 0)return;
	nPlayStatus = 0;
	timeKillEvent(timerID);
	polyphone = 0;
	for (int i = 0x00007BB0; i < 0x00007BBF; i += 0x00000001)
		midiOutShortMsg(hMidiOut, i);
}

void MidiPlayer::Stop(bool bResetMidi)
{
	Pause();
	if (bResetMidi)midiOutReset(hMidiOut);
	SetPos(0.0f);
	ZeroMemory(keyPressure, nChannels*nKeys*sizeof(*keyPressure));
}

bool MidiPlayer::SetLoop(float posStart, float posEnd)
{
	if (posStart > posEnd)
		return false;
	if (posEnd > midifile[0][nEventCount - 1].tick)
		return false;
	if (posStart < 0.0f)
		return false;
	if (!midifile[0].size())
		return false;
	loopStartTick = posStart;
	loopEndTick = posEnd;
	for (int i = 0; i < nEventCount; i++)
		if (midifile[0][i].tick >= loopStartTick)
		{
			nLoopStartEvent = i;
			break;
		}
	return true;
}

void MidiPlayer::SetVolume(unsigned v)
{
	v > MIDIPLAYER_MAX_VOLUME ? volume = MIDIPLAYER_MAX_VOLUME : volume = v;
}

bool MidiPlayer::SetPos(float pos)
{
	if (!midifile[0].size())
		return false;
	if (pos > midifile[0][nEventCount - 1].tick)
		return false;
	nextTick = pos;
	for (int i = 0; i < nEventCount; i++)
		if (midifile[0][i].tick >= nextTick)
		{
			nEvent = i;
			break;
		}
	return true;
}

void MidiPlayer::SetSendLongMsg(bool bSend)
{
	sendLongMsg = bSend;
}

void MidiPlayer::_TimerFunc(UINT wTimerID, UINT msg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	nextTick += midifile.getTicksPerQuarterNote()*deltaTime*tempo / 60000;
	if(nEvent < nEventCount)while (midifile[0][nEvent].tick <= nextTick)
	{
		nMsgSize = midifile[0][nEvent].size();
		if (midifile[0][nEvent].isTempo())
			tempo = (float)midifile[0][nEvent].getTempoBPM();
		midiEvent = 0;
		switch (nMsgSize)
		{
		case 4:midiEvent |= *(int*)midifile[0][nEvent].data() & 0xFF000000;
		case 3:midiEvent |= *(int*)midifile[0][nEvent].data() & 0x00FF0000;
		case 2:midiEvent |= *(int*)midifile[0][nEvent].data() & 0x0000FF00;
		case 1:midiEvent |= *(int*)midifile[0][nEvent].data() & 0x000000FF;
			switch (midiEvent & 0x000000F0)//获取MIDI消息类型
			{
			case 0x00000090://音符开
				/*第一字节（0x000000##）：MIDI消息类型，MIDI通道
				第二字节（0x0000##00）：音符编号（0～127，C4音的值为十进制60）
				第三字节（0x00##0000）：速度（强度，Velocity，0～127，0=音符关）*/
				SetKeyPressure(midiEvent & 0x0000000F, (midiEvent & 0x0000FF00) >> 8, (midiEvent & 0x00FF0000) >> 16);
				((BYTE*)&midiEvent)[2] = ((midiEvent >> 16) & 0x000000FF)*volume / MIDIPLAYER_MAX_VOLUME;
				polyphone++;
				break;
			case 0x00000080://音符关
				SetKeyPressure(midiEvent & 0x0000000F, (midiEvent & 0x0000FF00) >> 8, 0);
				polyphone--;
				break;
			}
			midiOutShortMsg(hMidiOut, midiEvent);
			break;
		default:
			if (sendLongMsg)
			{
				ZeroMemory(midiSysExMsg, sizeof(midiSysExMsg));
				for (size_t i = 0; i < nMsgSize; i++)
					midiSysExMsg[i] = midifile[0][nEvent].at(i);
				midiOutPrepareHeader(hMidiOut, &header, sizeof(header));
				midiOutLongMsg(hMidiOut, &header, sizeof(header));
				midiOutUnprepareHeader(hMidiOut, &header, sizeof(header));
			}
			break;
		}
		if (++nEvent >= nEventCount)break;
	}
	if (loopEndTick)
	{
		if (nextTick >= loopEndTick)
		{
			nextTick = loopStartTick;
			nEvent = nLoopStartEvent;
		}
	}
	else if (nEvent >= nEventCount)
	{
		Stop(false);
		return;
	}
}

int MidiPlayer::GetLastEventTick()
{
	if (!midifile[0].size())return 0;
	return midifile[0][nEventCount - 1].tick;
}

int MidiPlayer::GetPlayStatus()
{
	return nPlayStatus;
}

unsigned char MidiPlayer::GetKeyPressure(unsigned channel, unsigned key)
{
	if (channel >= nChannels || key >= nKeys)
		return false;
	return keyPressure[nKeys*channel + key];
}

int MidiPlayer::GetPosEventNum()
{
	return nEvent;
}

int MidiPlayer::GetPolyphone()
{
	return polyphone;
}

int MidiPlayer::GetQuarterNoteTicks()
{
	return midifile.getTicksPerQuarterNote();
}

float MidiPlayer::GetPosTick()
{
	return nextTick;
}

float MidiPlayer::GetBPM()
{
	return tempo;
}

double MidiPlayer::GetPosTimeInSeconds()
{
	return midifile.getTimeInSeconds((int)nextTick);
}
