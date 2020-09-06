#pragma once
#include"MidiFile.h"
#include<Windows.h>
#include<sstream>
#include<list>

#define MIDIPLAYER_MAX_VOLUME 100u

using namespace smf;

union RPNType
{
	struct RPNDivided
	{
		unsigned char lsb;
		unsigned char msb;
	}rpndivided;
	unsigned short rpnvalue;
};

enum MIDIMetaEventType
{
	MIDI_META_SEQUENCE_NUMBER = 0,
	MIDI_META_TEXT = 1,
	MIDI_META_COPYRIGHT = 2,
	MIDI_META_TRACK_NAME = 3,
	MIDI_META_INSTRUMENT_NAME = 4,
	MIDI_META_LYRICS = 5,
	MIDI_META_MARKER = 6,
	MIDI_META_CUE_POINT = 7,
	MIDI_META_CHANNEL_PREFIX = 32,
	MIDI_META_END_OF_TRACK = 47,
	MIDI_META_SET_TEMPO = 81,
	MIDI_META_SMPTE_OFFSET = 84,
	MIDI_META_BEATS = 88,
	MIDI_META_KEY = 89,
	MIDI_META_SEQUENCE_SPECIFIC = 127
};

struct MIDIMetaStructure
{
	BYTE metaMark;//0xFF
	BYTE midiMetaEventType;//可转换为MIDIMetaEventType枚举类型
	BYTE dataLength;//数据区长度
	BYTE *pData;//Meta数据区
	MIDIMetaStructure(BYTE *psrc)
	{
		metaMark = psrc[0];
		midiMetaEventType = psrc[1];
		dataLength = psrc[2];
		pData = new BYTE[dataLength];
		memcpy(pData, psrc + 3, dataLength);
	}
	~MIDIMetaStructure()
	{
		delete pData;
	}
	//参考：https://msdn.microsoft.com/library/dd293665.aspx
	MIDIMetaStructure(MIDIMetaStructure &&other)
	{
		metaMark = other.metaMark;
		midiMetaEventType = other.midiMetaEventType;
		dataLength = other.dataLength;
		pData = other.pData;
		other.pData = nullptr;
	}
};

typedef void(*MPOnProgramChangeFunc)(BYTE u4_channel, BYTE program);
typedef void(*MPOnFinishPlayFunc)(void*param);
typedef void(*MPOnControlChangeFunc)(BYTE u4_channel, BYTE cc, BYTE val);
typedef void(*MPOnSysExFunc)(PBYTE data, size_t length);

class MidiPlayer
{
public:
	//创建一个 MidiPlayer 类型的对象，参数为 MIDI 设备序号（从0算起），默认值为 MIDI_MAPPER
	MidiPlayer(unsigned deviceID = MIDI_MAPPER);
	//释放对象
	~MidiPlayer();
	//加载文件，如果失败返回 false，否则为 true
	bool LoadFile(const char*);
	//加载内存流，失败返回 false, 成功返回 true
	bool LoadStream(std::stringstream&);
	//释放序列资源
	void Unload();
	//参数1：true 从当前位置继续，false 从头开始，参数2：事件过多时是否丢弃
	bool Play(bool = true, bool = true);
	//暂停，参数：是否 panic
	void Pause(bool panic = true);
	//清除指定通道的声音，参数2为是否清空键盘状态，默认为否
	void Panic(unsigned channel, bool = false);
	//清除所有通道的声音，参数为是否清空键盘状态，默认为否
	void PanicAllChannel(bool = false);
	//停止，参数设置为 true 会执行 MIDI 重置操作
	void Stop(bool = true);
	//参数：循环开始点，循环结束点（以 tick 为单位），均为 0 表示不循环；
	//includeLeft: 左区间是否为闭区间（默认为开区间）
	//includeRight: 右区间是否为闭区间（默认为闭区间，该选项暂时不用）
	//设置值超过序列范围或不符合逻辑返回 false
	bool SetLoop(float, float, bool includeLeft = false, bool includeRight = true);
	//设置当前的播放位置，单位为 tick
	bool SetPos(float);
	//取值范围：[0, 100] %，大于 100 的视为 100
	void SetVolume(unsigned);
	//设置是否发送长消息（SysEx, MetaMsg 等）
	void SetSendLongMsg(bool);
	//获取 MIDI 序列最后一个事件的 tick 值（通常为 "End of Track" 事件）
	int GetLastEventTick();
	//获取播放状态，TRUE 为正在播放，FALSE 为没有播放
	int GetPlayStatus();
	//获取按键按下的力度，参数为 Channel 号和 Key 号
	unsigned char GetKeyPressure(unsigned, unsigned);
	//获取按键的弯音半音数，参数为 Channel 号
	float GetChannelPitchBend(unsigned);
	//获取当前事件的位置
	int GetPosEventNum();
	//获取复音数
	int GetPolyphone();
	//获取丢失事件数
	int GetDrop();
	//获取四分音符所占的 ticks
	int GetQuarterNoteTicks();
	//获取当前的 tick
	float GetPosTick();
	//获取BPM
	float GetBPM();
	//获取时间
	double GetPosTimeInSeconds();
	//获取事件总数
	int GetEventCount();
	//获取小节拍数
	int GetStepsPerBar();
	//获取MIDI文件中的元数据信息，返回值为信息数量
	int GetMIDIMeta(std::list<MIDIMetaStructure> &outMetaList);
	//设置播放完成后的回调函数，参数为函数指针和参数指针
	void SetOnFinishPlay(MPOnFinishPlayFunc, void*);
	//设置音色变换的回调函数，参数为通道号和音色号
	void SetOnProgramChange(MPOnProgramChangeFunc);
	//设置SysEx的回调函数，参数为通道号和音色号
	void SetOnSysEx(MPOnSysExFunc);
	//设置CC变换的回调函数，参数为通道号，CC控制器编号，值
	void SetOnControlChange(MPOnControlChangeFunc);
	//设置播放速度
	void SetPlaybackSpeed(float);
	//获取播放速度
	float GetPlaybackSpeed();
	//设置启用/禁用通道
	void SetChannelEnabled(unsigned channel, bool enabled);
	//获取启用/禁用通道
	bool GetChannelEnabled(unsigned channel);
protected:
	static MidiPlayer* _pObj;
	void _TimerFunc(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
private:
	void VarReset(bool = true);
	void SetKeyPressure(unsigned, unsigned, unsigned char);
	void SetChannelPitchBendFromRaw(unsigned, unsigned short);
	void SetChannelPitchBendRange(unsigned, unsigned char);
	MPOnFinishPlayFunc pFuncOnFinishPlay;
	void* paramOnFinishPlay;
	MPOnProgramChangeFunc pFuncOnProgramChange;
	MPOnControlChangeFunc pFuncOnControlChange;
	MPOnSysExFunc pFuncOnSysEx;
	bool sendLongMsg;
	float nextTick;
	float loopStartTick;
	float loopEndTick;
	bool loopIncludeLeft, loopIncludeRight;
	float tempo;
	int timerID;
	size_t nEvent;
	size_t nEventCount;
	int nLoopStartEvent;
	int nPlayStatus;//0=停止或暂停，1=播放
	bool bPlayDropEvents;
	MidiFile midifile;
	BYTE *midiSysExMsg;
	HMIDIOUT hMidiOut;
	unsigned char *keyPressure;
	unsigned short *channelPitchBend;//0x0000-0x2000-0x3FFF(-8192 - 0 - +8191)
	unsigned char *channelPitchSensitivity;//弯音幅度，半音数，0x00-0x18（0到24个半音）
	int polyphone;
	float playbackSpeed;
	bool *channelEnabled;

	int deltaTime;
	int stepsperbar;
	RPNType rpn;
	const unsigned nMaxSysExMsg;
	const unsigned nChannels;
	const unsigned nKeys;

	UINT timerInterval;
	std::vector<int> _mfNextTickEvents;
	int dropEventsCount;
};
