﻿#define _CRT_SECURE_NO_WARNINGS

#include "MidiPlayer.h"
#include "vstplugin.h"
#include "SF2Plugin.h"
#include <fstream>
#include <sstream>

#define INIT_ARRAY(pointer,n,var) for(size_t i=0;i<(n);i++)(pointer)[i]=(var)
#define DELETE_PTR(p) delete p;p=nullptr

MidiPlayer* MidiPlayer::_pObj = nullptr;

MidiPlayer::MidiPlayer(unsigned _deviceID,void*extraInfo) :sendLongMsg(true), timerID(0), deltaTime(20),
midiSysExMsg(nullptr), nMaxSysExMsg(256), nChannels(16), nKeys(128), rpn({ 255,255 }),
pFuncOnFinishPlay(nullptr), paramOnFinishPlay(nullptr), pFuncOnProgramChange(nullptr),
pFuncOnControlChange(nullptr), pFuncOnSysEx(nullptr), pFuncOnLyricText(nullptr),pluginPlayer(nullptr),initResult(0)
{
	if (MidiPlayer::_pObj)delete MidiPlayer::_pObj;
	MidiPlayer::_pObj = this;
	midiSysExMsg = new BYTE[nMaxSysExMsg];
	keyPressure = new unsigned char[nChannels*nKeys];
	channelPitchBend = new unsigned short[nChannels];
	channelPitchSensitivity = new unsigned char[nChannels];
	channelEnabled = new bool[nChannels];
	VarReset();
	deviceID = _deviceID;
	switch (deviceID)
	{
	case MIDI_DEVICE_USE_VST_PLUGIN:
		pluginPlayer = new VstPlugin;
		if (pluginPlayer->InitPlayer(2, 44100))
		{
			DELETE_PTR(pluginPlayer);
			initResult = -1;
			return;
		}
		if (pluginPlayer->LoadPlugin((LPCTSTR)extraInfo, 44100))
		{
			pluginPlayer->ReleasePlayer();
			DELETE_PTR(pluginPlayer);
			initResult = -1;
			return;
		}
		pluginPlayer->StartPlayback();
		break;
	case MIDI_DEVICE_USE_SOUNDFONT2:
		//TODO
		initResult = -1;
		return;
		break;
	default:
		if (midiOutOpen(&hMidiOut, deviceID, 0, 0, 0) != MMSYSERR_NOERROR)
			throw deviceID;
		SetVolume(MIDIPLAYER_MAX_VOLUME);
		break;
	}
}

MidiPlayer::~MidiPlayer()
{
	Stop();
	Unload();
	if (pluginPlayer)
	{
		pluginPlayer->StopPlayback();
		pluginPlayer->ReleasePlugin();
		pluginPlayer->ReleasePlayer();
		DELETE_PTR(pluginPlayer);
	}
	else
	{
		midiOutClose(hMidiOut);
	}
	delete[]channelEnabled;
	delete[]channelPitchSensitivity;
	delete[]channelPitchBend;
	delete[]keyPressure;
	delete[]midiSysExMsg;
	MidiPlayer::_pObj = nullptr;
}

void MidiPlayer::MidiOutShortMsgDispatch(DWORD midiEvent)
{
	if (pluginPlayer)
		pluginPlayer->SendMidiData(midiEvent);
	else
		midiOutShortMsg(hMidiOut, midiEvent);
}

void MidiPlayer::VarReset(bool doStop)
{
	for (unsigned i = 0; i < nChannels; i++)
		SetChannelEnabled(i, true);
	playbackSpeed = 1.0f;
	loopStartTick = loopEndTick = 0.0f;
	tempo = 120.0f;
	nextTick = 0.0f;
	nEvent = 0;
	nEventCount = 0;
	nLoopStartEvent = 0;
	nPlayStatus = 0;
	polyphone = 0;
	stepsperbar = 4;
	loopIncludeLeft = false;
	loopIncludeRight = true;
	if (doStop)Stop();
}

void MidiPlayer::SetKeyPressure(unsigned channel, unsigned key, unsigned char pressure)
{
	if (channel < nChannels&&key < nKeys)
	{
		key = nKeys*channel + key;
		if (pressure&&!keyPressure[key])polyphone++;
		else if (!pressure&&keyPressure[key])polyphone--;
		keyPressure[key] = pressure;
	}
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
	char magic[4];
	mem.read(magic, ARRAYSIZE(magic));
	if (strncmp(magic, "RIFF", ARRAYSIZE(magic)) == 0)
	{
		struct RMIDHeader
		{
			char strRIFF[4];
			int chunkSize;
			char strFormat[4];
			char strdata[4];
			int midiFileChunkSize;
		}rmi;
		mem.seekg(0);
		mem.read((char*)&rmi, sizeof rmi);
		std::string memstr = mem.str();
		memstr = memstr.substr(sizeof rmi, rmi.midiFileChunkSize);
		mem.str(memstr);
	}
	else mem.seekg(0);
	//read使用的是不带const的变量
	if (!midifile.read(mem))return false;
	VarReset(false);
	midifile.joinTracks();
	nEventCount = midifile[0].size();

	//构建nextTickEvents表
	_mfNextTickEvents.clear();
	float cTempo = tempo;
	while (_mfNextTickEvents.size() < nEventCount)
	{
		int cTick = midifile[0][_mfNextTickEvents.size()].tick;
		int cNextTick = (int)(cTick + midifile.getTicksPerQuarterNote() * deltaTime * cTempo / 60000);
		if (midifile[0][nEvent].isMeta() && midifile[0][nEvent].isTempo())
			cTempo = (float)midifile[0][nEvent].getTempoBPM();
		for (size_t cEvent = _mfNextTickEvents.size();; cEvent++)
		{
			if (cEvent >= nEventCount)
			{
				for (size_t left = nEventCount - _mfNextTickEvents.size(); left; left--)
					_mfNextTickEvents.push_back(nEventCount);
				break;
			}
			else if (midifile[0][cEvent].tick >= cNextTick)
			{
				for (size_t left = cEvent - _mfNextTickEvents.size(); left; left--)
					_mfNextTickEvents.push_back(cEvent);
				break;
			}
		}
	}
	return true;
}

void MidiPlayer::Unload()
{
	midifile.clear();
}

bool MidiPlayer::Play(bool goOn,bool dropEvents)
{
	if (nPlayStatus == 1)return true;
	if (!midifile[0].size())return false;
	if (!goOn)Stop();
	timerInterval = (UINT)(deltaTime / playbackSpeed);
	timerID = timeSetEvent(timerInterval, timerInterval, [](UINT wTimerID, UINT msg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
	{
		MidiPlayer::_pObj->_TimerFunc(wTimerID, msg, dwUser, dw1, dw2);
	}, 1, TIME_PERIODIC);
	nPlayStatus = 1;
	bPlayDropEvents = dropEvents;
	return true;
}

void MidiPlayer::Pause(bool panic)
{
	if (nPlayStatus == 0)return;
	nPlayStatus = 0;
	timeKillEvent(timerID);
	if (panic)
		PanicAllChannel();
}

void MidiPlayer::Panic(unsigned channel, bool resetkeyboard)
{
	int cmds[] = { 0x000078B0,//All Sounds Off
		//0x000079B0,//Reset All Controller
		0x00007BB0,//All Note Off
		//0x00007CB0//Omni Off
	};
	for (int i = 0; i < ARRAYSIZE(cmds); i++)
	{
		int midiData = cmds[i] | (channel & 0xF);
		if (pluginPlayer)
		{
			pluginPlayer->SendMidiData(midiData);
		}
		else
		{
			midiOutShortMsg(hMidiOut, midiData);
		}
	}
	if (resetkeyboard)
	{
		for (unsigned i = 0; i < nKeys; i++)
			SetKeyPressure(channel, i, 0);
	}
}

void MidiPlayer::PanicAllChannel(bool resetkeyboard)
{
	for (unsigned i = 0; i < nChannels; i++)
		Panic(i, resetkeyboard);
	if (pluginPlayer)
		pluginPlayer->CommitSend();
}

void MidiPlayer::Stop(bool bResetMidi)
{
	Pause();
	polyphone = 0;
	dropEventsCount = 0;
	if (bResetMidi)
	{
		if (pluginPlayer)
		{
			/* HOW TO DO THIS??? */
		}
		else
		{
			midiOutReset(hMidiOut);
		}
	}
	SetPos(0.0f);
	ZeroMemory(keyPressure, nChannels*nKeys*sizeof(*keyPressure));
	INIT_ARRAY(channelPitchBend, nChannels, 0x2000);
	INIT_ARRAY(channelPitchSensitivity, nChannels, 2);
}

bool MidiPlayer::SetLoop(float posStart, float posEnd, bool includeLeft, bool includeRight)
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
	loopIncludeLeft = includeLeft;
	loopIncludeRight = includeRight;
	for (size_t i = 0; i < nEventCount; i++)
		if (loopIncludeLeft ? midifile[0][i].tick >= loopStartTick : midifile[0][i].tick > loopStartTick)
		{
			nLoopStartEvent = i;
			break;
		}
	return true;
}

void MidiPlayer::SetVolume(unsigned v)
{
	if (pluginPlayer)
	{
		pluginPlayer->SetVolume(v / 100.0f);
	}
	else
	{
		//https://msdn.microsoft.com/zh-cn/library/windows/desktop/dd798480.aspx
		midiOutSetVolume(hMidiOut, MAKELONG(0xFFFF * v / MIDIPLAYER_MAX_VOLUME, 0xFFFF * v / MIDIPLAYER_MAX_VOLUME));
	}
}

bool MidiPlayer::SetPos(float pos)
{
	if (!midifile[0].size())
		return false;
	if (pos > midifile[0][nEventCount - 1].tick)
		return false;
	if (pos < 0.0f)
		return false;
	nextTick = pos;
	for (size_t i = 0; i < nEventCount; i++)
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
	//本来是想在这个 while 里判断右循环是否为闭区间着，
	//但突然一想觉得既要考虑带有判断的循环问题又要考虑这是否会影响结尾事件的发送所以还是暂时先放着吧。
	if (nEvent < nEventCount)
	{
		DWORD timeTick = GetTickCount();
		while (midifile[0][nEvent].tick <= nextTick)
		{
			if (bPlayDropEvents && GetTickCount() - timeTick > timerInterval)
			{
				dropEventsCount += _mfNextTickEvents[nEvent] - nEvent;
				nEvent = _mfNextTickEvents[nEvent];
				break;
			}
			DWORD midiEvent = *(DWORD*)midifile[0][nEvent].data();
			PBYTE b = midifile[0][nEvent].data();
			if (channelEnabled[b[0] & 0xF])
			{
				switch (b[0] & 0xF0)//获取MIDI消息类型
				{
				case 0x80:case 0x90:case 0xA0:case 0xB0:case 0xC0:case 0xD0:case 0xE0:
					_ProcessMidiShortEventInternal(midiEvent,true);
					break;
				case 0xF0:
					if (midifile[0][nEvent].isMeta())//不清楚这样行不行
					{
						if (midifile[0][nEvent].isTempo())
							tempo = (float)midifile[0][nEvent].getTempoBPM();
						else if (midifile[0][nEvent].data()[1] == 0x58)
							stepsperbar = midifile[0][nEvent].data()[3];
						else if (midifile[0][nEvent].isLyricText() && pFuncOnLyricText)
							pFuncOnLyricText(MIDIMetaStructure(midifile[0][nEvent].data()));
					}
					else if (sendLongMsg)
					{
						size_t nMsgSize = midifile[0][nEvent].size();
						for (size_t i = 0; i < nMsgSize; i++)
							midiSysExMsg[i] = midifile[0][nEvent][i];
						MIDIHDR header{};
						header.lpData = (LPSTR)midiSysExMsg;
						header.dwFlags = 0;
						header.dwBufferLength = nMsgSize;
						_ProcessMidiLongEventInternal(&header, true);
					}
					break;
				}
			}
			if (++nEvent >= nEventCount)
				break;
		}
		if (pluginPlayer)
			((VstPlugin*)pluginPlayer)->CommitSend();
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
		if (pFuncOnFinishPlay)
			pFuncOnFinishPlay(paramOnFinishPlay);
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

int MidiPlayer::GetDrop()
{
	return dropEventsCount;
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

void MidiPlayer::SetChannelPitchBendFromRaw(unsigned channel, unsigned short pitch)
{
	channelPitchBend[channel] = pitch & 0x7F00;
	channelPitchBend[channel] >>= 1;
	channelPitchBend[channel] |= pitch & 0x007F;
}

float MidiPlayer::GetChannelPitchBend(unsigned channel)
{
	return channelPitchSensitivity[channel] * ((float)channelPitchBend[channel] - 8192) / 8192;
}

void MidiPlayer::SetChannelPitchBendRange(unsigned channel, unsigned char range)
{
	channelPitchSensitivity[channel] = range;
}

int MidiPlayer::GetEventCount()
{
	return nEventCount;
}

void MidiPlayer::SetOnFinishPlay(MPOnFinishPlayFunc func, void* param)
{
	pFuncOnFinishPlay = func;
	paramOnFinishPlay = param;
}

int MidiPlayer::GetStepsPerBar()
{
	return stepsperbar;
}

int MidiPlayer::GetMIDIMeta(std::list<MIDIMetaStructure> &metalist)
{
	size_t i = 0;
	for (; i < nEventCount; i++)
		if (midifile[0][i].isMeta())
			metalist.push_back(MIDIMetaStructure(midifile[0][i].data()));
	return i;
}

void MidiPlayer::SetOnProgramChange(MPOnProgramChangeFunc f)
{
	pFuncOnProgramChange = f;
}

void MidiPlayer::SetOnControlChange(MPOnControlChangeFunc f)
{
	pFuncOnControlChange = f;
}

void MidiPlayer::SetOnSysEx(MPOnSysExFunc f)
{
	pFuncOnSysEx = f;
}

void MidiPlayer::SetOnLyricText(MPOnLyricText f)
{
	pFuncOnLyricText = f;
}

void MidiPlayer::SetPlaybackSpeed(float speed)
{
	playbackSpeed = speed;
	if (GetPlayStatus())
	{
		Pause(false);
		Play(true, bPlayDropEvents);
	}
}

float MidiPlayer::GetPlaybackSpeed()
{
	return playbackSpeed;
}

void MidiPlayer::SetChannelEnabled(unsigned channel, bool enabled)
{
	channelEnabled[channel] = enabled;
	if (channelEnabled[channel] == false)
	{
		for (unsigned i = 0; i < nKeys; i++)
			SetKeyPressure(channel, i, 0);
		Panic(channel, true);
	}
}

bool MidiPlayer::GetChannelEnabled(unsigned channel)
{
	return channelEnabled[channel];
}

HMIDIOUT MidiPlayer::GetHandle()
{
	return hMidiOut;
}

void MidiPlayer::_ProcessMidiShortEventInternal(DWORD midiEvent, bool sendMidiOut)
{
	PBYTE b = (PBYTE)&midiEvent;
	switch (b[0] & 0xF0)//获取MIDI消息类型
	{
	case 0x90://音符开
		/*第一字节（0x000000##）：MIDI消息类型，MIDI通道
		第二字节（0x0000##00）：音符编号（0～127，C4音的值为十进制60）
		第三字节（0x00##0000）：速度（强度，Velocity，0～127，0=音符关）*/
		SetKeyPressure(b[0] & 0xF, b[1], b[2]);
		if (sendMidiOut)
			MidiOutShortMsgDispatch(midiEvent);
		break;
	case 0x80://音符关
		SetKeyPressure(b[0] & 0xF, b[1], 0);
		if (sendMidiOut)
			MidiOutShortMsgDispatch(midiEvent);
		break;
	case 0xA0:
	case 0xD0:
		if (sendMidiOut)
			MidiOutShortMsgDispatch(midiEvent);
		break;
	case 0xC0://音色变换
		if (pFuncOnProgramChange)
			pFuncOnProgramChange(b[0] & 0xF, b[1]);
		if (sendMidiOut)
			MidiOutShortMsgDispatch(midiEvent);
		break;
	case 0xE0://弯音
		SetChannelPitchBendFromRaw(b[0] & 0xF, *(PWORD)(b + 1));
		if (sendMidiOut)
			MidiOutShortMsgDispatch(midiEvent);
		break;
	case 0xB0://CC控制器
		switch (b[1])
		{
		case 0x65://设置RPN的高位
			rpn.rpndivided.msb = b[2];
			break;
		case 0x64://设置RPN的低位
			rpn.rpndivided.lsb = b[2];
			break;
		case 0x6://向RPN表示的参数高位写入数据
			if (rpn.rpndivided.msb == 0)//判断RPN高位是不是表示弯音参数
				SetChannelPitchBendRange(b[0] & 0xF, b[2]);
			break;
			/*case 0x26://向RPN表示的参数低位写入数据
				if (rpn.rpndivided.lsb == 0)//判断RPN低位是不是表示弯音参数
					SetChannelPitchBendRange(b[0] & 0xF, b[3]);//因为弯音只用了高位字节所以就不用管低位了。
				break;*/
		}
		if (pFuncOnControlChange)
			pFuncOnControlChange(b[0] & 0xF, b[1], b[2]);
		if (sendMidiOut)
			MidiOutShortMsgDispatch(midiEvent);
		break;
	}
}

void MidiPlayer::_ProcessMidiLongEventInternal(LPMIDIHDR midiHeader, bool sendMidiOut)
{
	if (pFuncOnSysEx)
		pFuncOnSysEx((BYTE*)midiHeader->lpData, midiHeader->dwBufferLength);
	if (sendMidiOut)
	{
		if (pluginPlayer)
		{
			pluginPlayer->SendSysExData(midiHeader->lpData, midiHeader->dwBufferLength);
		}
		else
		{
			midiOutPrepareHeader(hMidiOut, midiHeader, sizeof(*midiHeader));
			midiOutLongMsg(hMidiOut, midiHeader, sizeof(*midiHeader));
			midiOutUnprepareHeader(hMidiOut, midiHeader, sizeof(*midiHeader));
		}
	}
}

void MidiPlayer::_ProcessMidiShortEvent(DWORD midiEvent, bool sendMidiOut)
{
	_ProcessMidiShortEventInternal(midiEvent, sendMidiOut);
	if (sendMidiOut&&pluginPlayer)
		((VstPlugin*)pluginPlayer)->CommitSend();
}

void MidiPlayer::_ProcessMidiLongEvent(LPMIDIHDR midiHeader, bool sendMidiOut)
{
	_ProcessMidiLongEventInternal(midiHeader, sendMidiOut);
	if (sendMidiOut&&pluginPlayer)
		((VstPlugin*)pluginPlayer)->CommitSend();
}

UINT MidiPlayer::GetDeviceID()
{
	return deviceID;
}

PluginWithWavePlayback* MidiPlayer::GetPlugin()
{
	return pluginPlayer;
}

int MidiPlayer::GetInitResult()
{
	return initResult;
}

int MidiPlayer::PluginExportToWav(LPCTSTR midiFilePath, LPCTSTR wavFilePath, LPVOID extraInfo)
{
	return pluginPlayer->ExportToWav(midiFilePath, wavFilePath, extraInfo);
}
