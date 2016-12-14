#pragma once
#include"MidiFile.h"
#include<Windows.h>
#include<sstream>

#define MIDIPLAYER_MAX_VOLUME 100u

union RPNType
{
	struct RPNDivided
	{
		unsigned char lsb;
		unsigned char msb;
	}rpndivided;
	unsigned short rpnvalue;
};

class MidiPlayer
{
public:
	//创建一个 MidiPlayer 类型的对象
	MidiPlayer();
	//释放对象
	~MidiPlayer();
	//加载文件，如果失败返回 false，否则为 true
	bool LoadFile(const char*);
	//加载内存流，失败返回 false, 成功返回 true
	bool LoadStream(std::stringstream&);
	//释放序列资源
	void Unload();
	//参数：true 从当前位置继续，false 从头开始
	bool Play(bool = true);
	//暂停
	void Pause();
	//停止，参数设置为 true 会执行 MIDI 重置操作
	void Stop(bool = true);
	//参数：循环开始点，循环结束点（以 tick 为单位），均为 0 表示不循环；
	//设置值超过序列范围或不符合逻辑返回 false
	bool SetLoop(float, float);
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
	//【注意】实测是预期数的2倍，暂无确定信息
	float GetChannelPitchBend(unsigned);
	//获取当前事件的位置
	int GetPosEventNum();
	//获取复音数
	int GetPolyphone();
	//获取四分音符所占的 ticks
	int GetQuarterNoteTicks();
	//获取当前的 tick
	float GetPosTick();
	//获取BPM
	float GetBPM();
	//获取时间
	double GetPosTimeInSeconds();

	//用于将回调函数操作导入类中，请不要调用此函数
	void _TimerFunc(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
private:
	void VarReset();
	void SetKeyPressure(unsigned, unsigned, unsigned char);
	void SetChannelPitchBendFromRaw(unsigned, unsigned short);
	void SetChannelPitchBendRange(unsigned, unsigned char);
	bool sendLongMsg;
	unsigned volume;
	float nextTick;
	float loopStartTick;
	float loopEndTick;
	float tempo;
	int timerID;
	int nEvent;
	int nEventCount;
	size_t nMsgSize;
	int midiEvent;
	int nLoopStartEvent;
	int nPlayStatus;//0=停止或暂停，1=播放
	MidiFile midifile;
	BYTE *midiSysExMsg;
	HMIDIOUT hMidiOut;
	MIDIHDR header;
	unsigned char *keyPressure;
	unsigned short *channelPitchBend;//0x0000-0x2000-0x3FFF(-8192 - 0 - +8191)
	unsigned char *channelPitchSensitivity;//弯音幅度，半音数，0x00-0x18（0到24个半音）
	int polyphone;

	int deltaTime;
	RPNType rpn;
	const unsigned nMaxSysExMsg;
	const unsigned nChannels;
	const unsigned nKeys;
};
