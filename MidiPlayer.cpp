#include "MidiPlayer.h"
#include <fstream>
#include <sstream>

#define INIT_ARRAY(pointer,n,var) for(size_t i=0;i<(n);i++)(pointer)[i]=(var)

MidiPlayer* MidiPlayer::_pObj = nullptr;

MidiPlayer::MidiPlayer(unsigned deviceID) :sendLongMsg(true), timerID(0), deltaTime(10),
midiSysExMsg(nullptr), nMaxSysExMsg(256), nChannels(16), nKeys(128), rpn({ 255,255 }),
pFuncOnFinishPlay(nullptr), paramOnFinishPlay(nullptr), pFuncOnProgramChange(nullptr)
{
	if (MidiPlayer::_pObj)delete MidiPlayer::_pObj;
	MidiPlayer::_pObj = this;
	midiSysExMsg = new BYTE[nMaxSysExMsg];
	keyPressure = new unsigned char[nChannels*nKeys];
	channelPitchBend = new unsigned short[nChannels];
	channelPitchSensitivity = new unsigned char[nChannels];
	VarReset();
	midiOutOpen(&hMidiOut, deviceID, 0, 0, 0);
	SetVolume(MIDIPLAYER_MAX_VOLUME);
}

MidiPlayer::~MidiPlayer()
{
	Stop();
	Unload();
	midiOutClose(hMidiOut);
	delete[]channelPitchSensitivity;
	delete[]channelPitchBend;
	delete[]keyPressure;
	delete[]midiSysExMsg;
	MidiPlayer::_pObj = nullptr;
}

void MidiPlayer::VarReset(bool doStop)
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
	timerID = timeSetEvent(deltaTime, 1, [](UINT wTimerID, UINT msg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
	{
		MidiPlayer::_pObj->_TimerFunc(wTimerID, msg, dwUser, dw1, dw2);
	}, 1, TIME_PERIODIC);
	nPlayStatus = 1;
	return true;
}

void MidiPlayer::Pause()
{
	if (nPlayStatus == 0)return;
	nPlayStatus = 0;
	timeKillEvent(timerID);
	Panic();
}

void MidiPlayer::Panic(bool resetkeyboard)
{
	for (int i = 0x00007BB0; i < 0x00007BBF; i += 0x00000001)
		midiOutShortMsg(hMidiOut, i);
	if (resetkeyboard)
	{
		polyphone = 0;
		ZeroMemory(keyPressure, nChannels*nKeys*sizeof(*keyPressure));
	}
}

void MidiPlayer::Stop(bool bResetMidi)
{
	Pause();
	polyphone = 0;
	if (bResetMidi)midiOutReset(hMidiOut);
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
	for (int i = 0; i < nEventCount; i++)
		if (loopIncludeLeft ? midifile[0][i].tick >= loopStartTick : midifile[0][i].tick > loopStartTick)
		{
			nLoopStartEvent = i;
			break;
		}
	return true;
}

void MidiPlayer::SetVolume(unsigned v)
{
	//https://msdn.microsoft.com/zh-cn/library/windows/desktop/dd798480.aspx
	midiOutSetVolume(hMidiOut, MAKELONG(0xFFFF * v / MIDIPLAYER_MAX_VOLUME, 0xFFFF * v / MIDIPLAYER_MAX_VOLUME));
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
	//本来是想在这个 while 里判断右循环是否为闭区间着，
	//但突然一想觉得既要考虑带有判断的循环问题又要考虑这是否会影响结尾事件的发送所以还是暂时先放着吧。
	if(nEvent < nEventCount)while (midifile[0][nEvent].tick <= nextTick)
	{
		nMsgSize = (int)midifile[0][nEvent].size();
		midiEvent = 0;
		switch (nMsgSize)
		{
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
				break;
			case 0x00000080://音符关
				SetKeyPressure(midiEvent & 0x0000000F, (midiEvent & 0x0000FF00) >> 8, 0);
				break;
			case 0x000000C0://音色变换
				if (pFuncOnProgramChange)
					pFuncOnProgramChange(midiEvent & 0x0000000F, (midiEvent & 0x0000FF00) >> 8);
				break;
			case 0x000000E0://弯音
				SetChannelPitchBendFromRaw(midiEvent & 0x0000000F, (midiEvent & 0x00FFFF00) >> 8);
				break;
			case 0x000000B0://CC控制器
				switch (midiEvent & 0x0000FF00)
				{
				case 0x00006500://设置RPN的高位
					rpn.rpndivided.msb = (midiEvent & 0x00FF0000) >> 16;
					break;
				case 0x00006400://设置RPN的低位
					rpn.rpndivided.lsb = (midiEvent & 0x00FF0000) >> 16;
					break;
				case 0x00000600://向RPN表示的参数高位写入数据
					if (rpn.rpndivided.msb == 0)//判断RPN高位是不是表示弯音参数
						SetChannelPitchBendRange(midiEvent & 0x0000000F, (midiEvent & 0x00FF0000) >> 16);
					break;
				/*case 0x00002600://向RPN表示的参数低位写入数据
					if (rpn.rpndivided.lsb == 0)//判断RPN低位是不是表示弯音参数
						SetChannelPitchBendRange(midiEvent & 0x0000000F, (midiEvent & 0x00FF0000) >> 24);//因为弯音只用了高位字节所以就不用管低位了。
					break;*/
				}
				break;
			}
			midiOutShortMsg(hMidiOut, midiEvent);
			break;
		default:
			if (midifile[0][nEvent].isMeta())//不清楚这样行不行
			{
				if (midifile[0][nEvent].isTempo())
					tempo = (float)midifile[0][nEvent].getTempoBPM();
				else if (midifile[0][nEvent].data()[1] == 0x58)
					stepsperbar = midifile[0][nEvent].data()[3];
			}
			else if (sendLongMsg)
			{
				for (int i = 0; i < nMsgSize; i++)
					midiSysExMsg[i] = midifile[0][nEvent][i];
				ZeroMemory(&header, sizeof header);
				header.lpData = (LPSTR)midiSysExMsg;
				header.dwFlags = 0;
				header.dwBufferLength = nMsgSize;
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
	int i = 0;
	for (; i < nEventCount; i++)
		if (midifile[0][i].isMeta())
			metalist.push_back(MIDIMetaStructure(midifile[0][i].data()));
	return i;
}

void MidiPlayer::SetOnProgramChange(MPOnProgramChangeFunc f)
{
	pFuncOnProgramChange = f;
}
