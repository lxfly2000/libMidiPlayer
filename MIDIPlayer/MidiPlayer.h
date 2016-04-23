#pragma once
#include"MidiFile.h"
#include<Windows.h>

class MidiPlayer
{
public:
	MidiPlayer();
	~MidiPlayer();
	MidiPlayer(const MidiPlayer&) = delete;
	//�����ļ������ʧ�ܷ��� false������Ϊ true
	bool LoadFile(const char*);
	void Unload();
	//������true �ӵ�ǰλ�ü�����false ��ͷ��ʼ
	bool Play(bool = true);
	void Pause();
	void Stop(bool = true);
	//������ѭ����ʼ�㣬ѭ�������㣨�� tick Ϊ��λ������Ϊ 0 ��ʾ��ѭ����
	//����ֵ�������з�Χ�򲻷����߼����� false
	bool SetLoop(float, float);
	//��ע�⡿�ù���Ŀǰ�޷����� MIDI
	//ȡֵ��Χ��[0, 100] %������ 100 ����Ϊ 100
	void SetVolume(unsigned);
	//�����Ƿ��ͳ���Ϣ��SysEx, MetaMsg �ȣ�
	void SetSendLongMsg(bool);
	int GetLastEventTick();
	int GetPlayStatus();

	void TimerFunc(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
private:
	void VarReset();
	//���õ�ǰ�Ĳ���λ��
	bool SetPos(float);
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
	int nPlayStatus;//0=ֹͣ����ͣ��1=����
	MidiFile midifile;
	BYTE *midiSysExMsg;
	HMIDIOUT hMidiOut;
	MIDIHDR header;

	int deltaTime;
	const int nMaxSysExMsg = 256;
};
