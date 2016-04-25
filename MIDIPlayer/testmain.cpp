#include "MidiPlayer.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
	MidiPlayer player;
	string input;
	bool sendlong = true;
	float a = 0.0f, b = 0.0f;
	do
	{
		cout << "0=退出 1=输入文件名 2=播放 3=暂停 4=停止 5=设置循环 6=释放序列 "
			"7=切换是否发送长消息 8=检查是否播放中 9=结束点：";
		cin >> input;
		switch (atoi(input.c_str()))
		{
		case 1:
			getchar();
			getline(cin, input);
			player.LoadFile(input.c_str());
			break;
		case 2:
			player.Play(true);
			break;
		case 3:
			player.Pause();
			break;
		case 4:
			player.Stop();
			break;
		case 5:
			cin >> a >> b;
			player.SetLoop(a, b);
			break;
		case 6:
			player.Unload();
			break;
		case 7:
			sendlong = !sendlong;
			player.SetSendLongMsg(sendlong);
			cout << "发送长消息：" << (sendlong ? "ON" : "OFF") << endl;
			cout << "* 当你听到系统演奏的音符有些奇怪时建议尝试修改此选项。\n";
			break;
		case 8:
			cout << (player.GetPlayStatus() ? "正在播放。" : "没有播放。") << endl;
			break;
		case 9:
			cout << player.GetLastEventTick() << endl;
			break;
		case 0:
		default:break;
		}
	} while (input != "0");
}
