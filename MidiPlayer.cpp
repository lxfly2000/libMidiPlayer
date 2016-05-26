#include "MidiPlayer.h"

static MidiPlayer *pmp = nullptr;
void WINAPI OnTimerFunc(UINT wTimerID, UINT msg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	pmp->TimerFunc(wTimerID, msg, dwUser, dw1, dw2);
}

MidiPlayer::MidiPlayer() :volume(100), sendLongMsg(true), timerID(0), deltaTime(10), midiSysExMsg(nullptr),
nMaxSysExMsg(256), nChannels(16), nKeys(128)
{
	if (pmp)delete pmp;
	pmp = this;
	midiSysExMsg = new BYTE[nMaxSysExMsg]{ 0 };
	keyPressed = new bool[nChannels*nKeys];
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
	delete[]keyPressed;
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
}

void MidiPlayer::SetKeyPressed(unsigned channel, unsigned key, bool bPressed)
{
	if (channel < nChannels&&key < nKeys)
		keyPressed[nKeys*channel + key] = bPressed;
}

bool MidiPlayer::LoadFile(const char *filename)
{
	Stop();
	Unload();
	if (!midifile.read(filename))return false;
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
	if (nPlayStatus == 1)
		timeKillEvent(timerID);
	for (int i = 0x00007BB0; i < 0x00007BBF; i += 0x00000001)
		midiOutShortMsg(hMidiOut, i);
	nPlayStatus = 0;
}

void MidiPlayer::Stop(bool bResetMidi)
{
	Pause();
	if (bResetMidi)midiOutReset(hMidiOut);
	SetPos(0.0f);
	ZeroMemory(keyPressed, nChannels*nKeys*sizeof(*keyPressed));
}

bool MidiPlayer::SetLoop(float posStart, float posEnd)
{
	if (posStart > posEnd)
		return false;
	if (loopEndTick > midifile[0][nEventCount - 1].tick)
		return false;
	if (loopStartTick < 0.0f)
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
	v > 100 ? volume = 100 : volume = v;
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

void MidiPlayer::TimerFunc(UINT wTimerID, UINT msg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	nextTick += midifile.getTicksPerQuarterNote()*deltaTime*tempo / 60000;
	if (loopEndTick > 0.0f&&nextTick >= loopEndTick)
	{
		nextTick = loopStartTick;
		nEvent = nLoopStartEvent;
	}
	while (midifile[0][nEvent].tick <= nextTick)
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
			switch (midiEvent & 0x000000F0)
			{
			case 0x00000090:SetKeyPressed(midiEvent & 0x0000000F, (midiEvent & 0x0000FF00) >> 8, (midiEvent & 0x00FF0000) != 0); break;
			case 0x00000080:SetKeyPressed(midiEvent & 0x0000000F, (midiEvent & 0x0000FF00) >> 8, false); break;
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
		if (nEvent >= nEventCount || midifile[0][nEvent].isEndOfTrack())
		{
			Stop(false);
			return;
		}
		nEvent++;
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

bool MidiPlayer::GetKeyPressed(unsigned channel, unsigned key)
{
	if (channel >= nChannels || key >= nKeys)
		return false;
	return keyPressed[nKeys*channel + key];
}