#include"MidiPlayer.h"
#include <iostream>
#pragma comment(lib,"winmm.lib")

int main(int argc, char** argv)
{
	MidiPlayer player;
	std::string input;
	bool sendlong = true;
	float a = 0.0f, b = 0.0f;
	do
	{
		std::cout << "0=退出 1=输入文件名 2=播放 3=暂停 4=停止 5=设置循环 6=释放序列 "
			"7=切换是否发送长消息 8=检查是否播放中：";
		std::cin >> input;
		switch (atoi(input.c_str()))
		{
		case 1:
			getchar();
			std::getline(std::cin, input);
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
			std::cin >> a >> b;
			player.SetLoop(a, b);
			break;
		case 6:
			player.Unload();
			break;
		case 7:
			sendlong = !sendlong;
			player.SetSendLongMsg(sendlong);
			std::cout << "发送长消息：" << (sendlong ? "ON" : "OFF") << endl;
			std::cout << "* 当你听到系统演奏的音符有些奇怪时建议尝试修改此选项。\n";
			break;
		case 8:
			std::cout << (player.GetPlayStatus() ? "正在播放。" : "没有播放。") << endl;
			break;
		case 0:
		default:break;
		}
	} while (input != "0");
}