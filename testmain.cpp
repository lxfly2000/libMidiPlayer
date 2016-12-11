﻿#include "MidiPlayer.h"
#include "Codes\ConsoleCursor.h"
#include <iostream>
#include <fstream>
#include <conio.h>

using namespace std;

int main(int argc, char *argv[])
{
	HANDLE hConsole;
	MidiPlayer player;
	string input;
	bool sendlong = false;
	float a = 0.0f, b = 0.0f;
	int status = 0;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD cursorsize;
	BOOL showcursor;
	
	GetCursorSize(&cursorsize, &showcursor);
	player.SetSendLongMsg(sendlong);
	if(argc == 2)
	{
		if (argv[1][0] == '\"')
		{
			argv[1][strlen(argv[1]) - 1] = '\0';
			player.LoadFile(argv[1] + 1);
		}
		else
		{
			player.LoadFile(argv[1]);
		}
		player.Play(true);
		cout << "正在播放：" << (argv[1][0] == '\"' ? argv[1] + 1 : argv[1]) << endl;
	}
	
	do
	{
		switch (status)
		{
		case 0:
			cout << "0=退出 1=输入文件名 2=" << (player.GetPlayStatus() ? "暂停" : "播放") <<
				" 3=停止 4=音量 5=设置循环 6=释放序列 7=切换是否发送长消息 8=检查是否播放中 9=结束点 a=键位：";
			cin >> input;
			switch (input[0])
			{
			case '1':
				cout << "文件名（相对或绝对路径，不要引号）：";
				getchar();
				getline(cin, input);
				if (player.LoadFile(input.c_str()))
				{
					ifstream loopfile(input + ".txt", ios::in);
					if (loopfile)
					{
						loopfile >> a >> b;
						player.SetLoop(a, b);
						cout << "自动设置循环为：" << a << ", " << b << endl;
					}
					loopfile.close();
				}
				break;
			case '2':
				player.GetPlayStatus() ? player.Pause() : player.Play(true);
				break;
			case '3':
				player.Stop(false);
				break;
			case '4':
				cout << "音量（范围 0～100）：";
				cin >> a;
				player.SetVolume((unsigned)a);
				break;
			case '5':
				cout << "输入循环起始，循环结束位置（单位 Tick）：";
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
				cout << player.GetLastEventTick() << " Tick" << endl;
				break;
			case 'a':
				ClearScreen(hConsole);
				cout << "按任意键退出键位状态。" << endl;
				status = 1;
				SetCursorSize(cursorsize, FALSE);
			default:break;
			}
			break;
		case 1:
			SetConsoleCursorPosition(hConsole, { 0i16,1i16 });
			for (int i = 0; i < 16; i++)
			{
				for (int j = 0; j < 108; j++)//108是为了避免在控制台中输出时换行
					cout << (player.GetKeyPressure(i, j) ? '#' : '.');
				cout << endl;
			}
			if (_kbhit())
			{
				status = 0;
				SetCursorSize(cursorsize, TRUE);
			}
			break;
		}
	} while (input != "0");
}