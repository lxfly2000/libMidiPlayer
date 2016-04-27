#pragma once
#include"MidiFile.h"
#include<Windows.h>

class MidiPlayer
{
public:
	MidiPlayer();
	~MidiPlayer();
	MidiPlayer(const MidiPlayer&) = delete;
	//加载文件，如果失败返回 false，否则为 true
	bool LoadFile(const char*);
	void Unload();
	//参数：true 从当前位置继续，false 从头开始
	bool Play(bool = true);
	void Pause();
	void Stop(bool = true);
	//参数：循环开始点，循环结束点（以 tick 为单位），均为 0 表示不循环；
	//设置值超过序列范围或不符合逻辑返回 false
	bool SetLoop(float, float);
	//设置当前的播放位置，单位为 tick
	bool SetPos(float);
	//【注意】该功能目前无法用于 MIDI
	//取值范围：[0, 100] %，大于 100 的视为 100
	void SetVolume(unsigned);
	//设置是否发送长消息（SysEx, MetaMsg 等）
	void SetSendLongMsg(bool);
	int GetLastEventTick();
	int GetPlayStatus();

	void TimerFunc(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
private:
	void VarReset();
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

	int deltaTime;
	const int nMaxSysExMsg = 256;
};
