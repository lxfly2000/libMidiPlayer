#include<Windows.h>
#include"ConsoleCursor.h"

//移动控制台中的光标至指定位置，第一个参数为横坐标，第二个参数为纵坐标。
void gotoXY(short x, short y)
{
	COORD coord = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void SetCursorSize(DWORD size, BOOL visible)
{
	CONSOLE_CURSOR_INFO CursorInfo = { size, visible };
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &CursorInfo);
}

void GetCursorSize(DWORD* psize, BOOL* pvisible)
{
	CONSOLE_CURSOR_INFO CursorInfo;
	GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &CursorInfo);
	*psize = CursorInfo.dwSize;
	*pvisible = CursorInfo.bVisible;
}

BOOL ClearScreen(HANDLE hConsole)
{
	COORD coordScreen = { 0,0 };//设置清屏后光标返回的屏幕左上角坐标
	BOOL bSuccess;
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;//保存缓冲区信息
	DWORD dwConSize;//当前缓冲区可容纳的字符数
	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);//获得缓冲区信息
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;//缓冲区容纳字符数目
	//用空格填充缓冲区
	bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR)' ', dwConSize, coordScreen, &cCharsWritten);
	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);//获得缓冲区信息
	//填充缓冲区属性
	bSuccess = FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
	//光标返回屏幕左上角坐标
	bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
	return bSuccess;
}

BOOL ClearScreen()
{
	return ClearScreen(GetStdHandle(STD_OUTPUT_HANDLE));
}