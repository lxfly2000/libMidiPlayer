#include "MidiPlayer.h"
#include "E:\Codes\控制台光标控制.h"
#include <iostream>
#include <conio.h>

using namespace std;

int main(int argc, char *argv[])
{
	HANDLE hConsole;
	MidiPlayer player;
	string input;
	bool sendlong = true;
	float a = 0.0f, b = 0.0f;
	int status = 0;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	do
	{
		switch (status)
		{
		case 0:
			cout << "0=退出 1=输入文件名 2=播放 3=暂停 4=停止 5=设置循环 6=释放序列 "
				"7=切换是否发送长消息 8=检查是否播放中 9=结束点 a=键位：";
			cin >> input;
			switch (input[0])
			{
			case '1':
				getchar();
				getline(cin, input);
				player.LoadFile(input.c_str());
				break;
			case '2':
				player.Play(true);
				break;
			case '3':
				player.Pause();
				break;
			case '4':
				player.Stop(false);
				break;
			case '5':
				cin >> a >> b;
				player.SetLoop(a, b);
				break;
			case '6':
				player.Stop(false);
				player.Unload();
				break;
			case '7':
				sendlong = !sendlong;
				player.SetSendLongMsg(sendlong);
				cout << "发送长消息：" << (sendlong ? "ON" : "OFF") << endl;
				cout << "* 当你听到系统演奏的音符有些奇怪时建议尝试修改此选项。\n";
				break;
			case '8':
				cout << (player.GetPlayStatus() ? "正在播放。" : "没有播放。") << endl;
				break;
			case '9':
				cout << player.GetLastEventTick() << endl;
				break;
			case 'a':
				ClearScreen(hConsole);
				cout << "按任意键退出键位状态。" << endl;
				status = 1;
			default:break;
			}
			break;
		case 1:
			SetConsoleCursorPosition(hConsole, { 0i16,1i16 });
			for (int i = 0; i < 16; i++)
			{
				for (int j = 0; j < 108; j++)
					cout << (player.GetKeyPressed(i, j) ? "#" : ".");
				cout << endl;
			}
			if (_kbhit())status = 0;
			break;
		}
	} while (input != "0");
}