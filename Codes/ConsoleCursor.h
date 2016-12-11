#pragma once

//移动控制台中的光标至指定位置，第一个参数为横坐标，第二个参数为纵坐标。
extern void gotoXY(short x, short y);

extern void SetCursorSize(DWORD size, BOOL visible);

extern void GetCursorSize(DWORD* psize, BOOL* pvisible);

extern BOOL ClearScreen(HANDLE hConsole);

extern BOOL ClearScreen();
