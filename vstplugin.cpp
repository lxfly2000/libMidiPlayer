#define _CRT_SECURE_NO_WARNINGS


#include "vstplugin.h"
#include <vector>
#include <atlconv.h>

#define KEYNAME_BUFFER_LENGTH	"BufferTime"
#define VDEFAULT_BUFFER_LENGTH	20



CVSTHost VstPlugin::vsthost;


int VstPlugin::LoadPlugin(LPCTSTR path,int smpRate)
{
	nEffect = vsthost.LoadPlugin(path);
	if (nEffect == -1)
		return -1;

	vsthost.EffOpen(nEffect);

	vsthost.SetSampleRate(smpRate);
	vsthost.SetBlockSize(512);

	vsthost.EffResume(nEffect);


    USES_CONVERSION;
    char effname[256];
    ERect effrect;
    ERect* peffrect = &effrect;
    vsthost.EffGetEffectName(nEffect, effname);
    vsthost.EffEditGetRect(nEffect, &peffrect);
    RECT rect{ peffrect->left,peffrect->top,peffrect->right,peffrect->bottom };
    WNDCLASSEX wcex{ sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW ,[](HWND hwnd,UINT msg,WPARAM w,LPARAM l)
        {
            if (msg == WM_CLOSE)
            {
                ((VstPlugin*)GetWindowLongPtr(hwnd,GWLP_USERDATA))->ShowPluginWindow(false);
                return (LRESULT)0;
            }
            return DefWindowProc(hwnd,msg,w,l);
    }};
    wcex.lpszClassName = TEXT("VSTWindow");
    RegisterClassEx(&wcex);
    DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRectEx(&rect, style, FALSE, NULL);
    hwndForVst = CreateWindowEx(NULL, wcex.lpszClassName, A2W(effname), style, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL);
    SetWindowLongPtr(hwndForVst, GWLP_USERDATA, (LONG_PTR)this);
    DWORD e = GetLastError();
    ShowPluginWindow(true);
    return 0;
}

int VstPlugin::ReleasePlugin()
{
    ShowPluginWindow(false);
    DestroyWindow(hwndForVst);
    hwndForVst = nullptr;
	vsthost.RemoveAll();
    return 0;
}

int VstPlugin::ShowPluginWindow(bool show)
{
    if (show)
    {
        ShowWindow(hwndForVst, SW_SHOW);
        vsthost.EffEditOpen(nEffect, hwndForVst);
        ERect effrect;
        ERect* peffrect = &effrect;
        vsthost.EffEditGetRect(nEffect, &peffrect);
        RECT rect{ peffrect->left,peffrect->top,peffrect->right,peffrect->bottom };
        AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, NULL);
        SetWindowPos(hwndForVst, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE);
    }
    else
    {
        vsthost.EffEditClose(nEffect);
        ShowWindow(hwndForVst, SW_HIDE);
    }
    return 0;
}

bool VstPlugin::IsPluginWindowShown()
{
	return vsthost.GetAt(nEffect)->bEditOpen;
}

int VstPlugin::SendMidiData(DWORD midiData)
{
    VstMidiEvent ve{};
    ve.type = kVstMidiType;
    ve.byteSize = sizeof(ve);
    ve.deltaFrames = 0;//这个属性有什么用？
    ve.flags = kVstMidiEventIsRealtime;
    memcpy(ve.midiData, &midiData, sizeof(ve.midiData));
    VstEvents ves{};
    ves.numEvents = 1;
    ves.events[0] = (VstEvent*)&ve;
	vsthost.EffProcessEvents(nEffect, &ves);
    return 0;
}

int VstPlugin::SendSysExData(LPVOID data, DWORD length)
{
    VstMidiSysexEvent ve{};
    ve.type = kVstMidiType;
    ve.byteSize = sizeof(ve);
    ve.deltaFrames = 2000;
    ve.flags = kVstMidiEventIsRealtime;
    ve.dumpBytes = length;
    memcpy(ve.sysexDump, data, length);
    VstEvents ves{};
    ves.numEvents = 1;
    ves.events[0] = (VstEvent*)&ve;
	vsthost.EffProcessEvents(nEffect, &ves);
    return 0;
}

void VstPlugin::_Playback()
{
    UINT buffer_time_ms = GetPrivateProfileInt(TEXT("XAudio2"), TEXT(KEYNAME_BUFFER_LENGTH), VDEFAULT_BUFFER_LENGTH, TEXT(".\\VisualMIDIPlayer.ini"));
    size_t bytesof_soundBuffer = player.GetSampleRate() * player.GetChannelCount() * player.GetBytesPerVar() * buffer_time_ms / 1000;
    short* buffer = reinterpret_cast<short*>(new BYTE[bytesof_soundBuffer]);

    size_t singleChSamples = player.GetSampleRate() * buffer_time_ms / 1000;
    std::vector<float*>pfChannels;
    long numOutputs = vsthost.GetAt(nEffect)->pEffect->numOutputs;//通道数必须至少为该参数的数值，否则极有可能报错
    for (int i = 0; i < numOutputs; i++)
        pfChannels.push_back(new float[singleChSamples]);
    while (isPlayingBack)
    {
		for (int i = 0; i < player.GetChannelCount(); i++)
			ZeroMemory(pfChannels[i], singleChSamples*sizeof(float));
		if (vsthost.GetAt(nEffect)->pEffect->flags&effFlagsCanReplacing)
			vsthost.EffProcessReplacing(nEffect, pfChannels.data(), pfChannels.data(), singleChSamples);
		else
			vsthost.EffProcess(nEffect, pfChannels.data(), pfChannels.data(), singleChSamples);
        for (size_t i = 0; i < singleChSamples; i++)
        {
            //此处必须保证bytesPerVar是4
            for (int j = 0; j < player.GetChannelCount(); j++)
                buffer[player.GetChannelCount() * i + j] = pfChannels[j][i] * 32767;
        }
        player.Play((BYTE*)buffer, bytesof_soundBuffer);
        player.WaitForBufferEndEvent();
    }
    for (int i = 0; i < numOutputs; i++)
        delete[]pfChannels[i];
    delete[]buffer;
}
