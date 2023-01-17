#include "common.h"

TCHAR profilePath[MAX_PATH];

LPCTSTR GetProfilePath()
{
	GetModuleFileName(GetModuleHandle(NULL), profilePath, ARRAYSIZE(profilePath));
	lstrcpy(wcsrchr(profilePath, '\\'), TEXT("\\MidiPlayer.ini"));
	return profilePath;
}
