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
		std::cout << "0=�˳� 1=�����ļ��� 2=���� 3=��ͣ 4=ֹͣ 5=����ѭ�� 6=�ͷ����� "
			"7=�л��Ƿ��ͳ���Ϣ 8=����Ƿ񲥷��У�";
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
			std::cout << "���ͳ���Ϣ��" << (sendlong ? "ON" : "OFF") << endl;
			std::cout << "* ��������ϵͳ�����������Щ���ʱ���鳢���޸Ĵ�ѡ�\n";
			break;
		case 8:
			std::cout << (player.GetPlayStatus() ? "���ڲ��š�" : "û�в��š�") << endl;
			break;
		case 0:
		default:break;
		}
	} while (input != "0");
}