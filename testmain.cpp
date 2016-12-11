#include "MidiPlayer.h"
#include "Codes\ConsoleCursor.h"
#include <iostream>
#include <fstream>
#include <conio.h>

bool LoadMIDI(MidiPlayer* pplayer, std::string fp)
{
	if (!pplayer->LoadFile(fp.c_str()))
		return false;
	std::ifstream loopfile(fp + ".txt", std::ios::in);
	if (loopfile)
	{
		float a, b;
		loopfile >> a >> b;
		pplayer->SetLoop(a, b);
		std::cout << "自动设置循环为：" << a << "," << b << std::endl;
	}
	loopfile.close();
	return true;
}

int main(int argc, char *argv[])
{
	HANDLE hConsole;
	HWND hCwindow;
	MidiPlayer player;
	string input;
	bool sendlong = false;
	float a = 0.0f, b = 0.0f;
	int status = 0;
	int millisec = 0, minute = 0, second = 0, bar = 0, step = 0, tick = 0;
	const int stepsperbar = 4;
	TCHAR stbuffer[80];
	DWORD cursorsize;
	BOOL showcursor;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	hCwindow = GetConsoleWindow();
	
	GetCursorSize(&cursorsize, &showcursor);
	player.SetSendLongMsg(sendlong);
	if(argc == 2)
	{
		if (argv[1][0] == '\"')
		{
			argv[1][strlen(argv[1]) - 1] = '\0';
			LoadMIDI(&player, argv[1] + 1);
		}
		else
		{
			LoadMIDI(&player, argv[1]);
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
				LoadMIDI(&player, input);
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
			millisec = (int)(player.GetPosTimeInSeconds()*1000.0);
			second = millisec / 1000;
			millisec %= 1000;
			minute = second / 60;
			second %= 60;
			tick = (int)player.GetPosTick();
			step = tick / player.GetQuarterNoteTicks();
			tick %= player.GetQuarterNoteTicks();
			bar = step / stepsperbar;
			step %= stepsperbar;
			swprintf_s(stbuffer, L"BPM:%5.3f 时间：%d:%02d.%03d Tick:%d:%02d:%03d 事件：%d 复音数：%d", player.GetBPM(),
				minute, second, millisec, bar + 1, step + 1, tick, player.GetPosEventNum(), player.GetPolyphone());
			SetWindowText(hCwindow, stbuffer);
			if (_kbhit())
			{
				status = 0;
				SetCursorSize(cursorsize, TRUE);
			}
			break;
		}
	} while (input != "0");
}