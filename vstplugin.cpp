//本人技术有限，该功能对部分VST插件支持效果不好，具体表现有MIDI延迟、部分插件无声音等

#define _CRT_SECURE_NO_WARNINGS


#include "vstplugin.h"
#include <vector>
#include <sstream>
#include <atlconv.h>
#include <MidiFile.h>

#include "SDKWaveFile.h"

#define KEYNAME_BUFFER_LENGTH	"BufferTime"
#define VDEFAULT_BUFFER_LENGTH	20



bool VstPluginHost::OnSizeWindow(int nEffect, long width, long height)
{
    RECT rect{ 0,0,width,height };
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, NULL);
    SetWindowPos((HWND)GetAt(nEffect)->pEffect->user, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
    return true;
}

VstPluginHost VstPlugin::vsthost;

#define ALWAYS_ON_TOP 1

int VstPlugin::LoadPlugin(LPCTSTR path,int smpRate)
{
	nEffect = vsthost.LoadPlugin(path);
	if (nEffect == -1)
		return -1;

	vsthost.EffOpen(nEffect);

    m_smpRate = smpRate;
	vsthost.SetSampleRate(smpRate);
    vsthost.SetBlockSize(m_blockSize = 512);

    vsthost.EffMainsChanged(nEffect, true);
    vsthost.EffStartProcess(nEffect);
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
            switch (msg)
            {
            case WM_CREATE:
                AppendMenu(GetSystemMenu(hwnd, FALSE), MF_STRING | MF_CHECKED, ALWAYS_ON_TOP, TEXT("置顶显示(&A)"));
                SetTimer(hwnd, 1, 20, NULL);
                break;
            case WM_DESTROY:
                KillTimer(hwnd, 1);
                break;
            case WM_CLOSE:
                ((VstPlugin*)GetWindowLongPtr(hwnd,GWLP_USERDATA))->ShowPluginWindow(false);
                return (LRESULT)0;
            case WM_SYSCOMMAND:
                if (w == ALWAYS_ON_TOP)
                {
                    DWORD exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
                    UINT flags = MF_BYCOMMAND;
                    if (exStyle & WS_EX_TOPMOST)
                    {
                        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    }
                    else
                    {
                        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                        flags |= MF_CHECKED;
                    }
                    ModifyMenu(GetSystemMenu(hwnd, FALSE), ALWAYS_ON_TOP, flags, ALWAYS_ON_TOP, TEXT("置顶显示(&A)"));
                }
                break;
            case WM_TIMER:
                ((VstPlugin*)GetWindowLongPtr(hwnd, GWLP_USERDATA))->OnIdle();
                break;
            }
            return DefWindowProc(hwnd,msg,w,l);
    }};
    wcex.lpszClassName = TEXT("VSTWindow");
    RegisterClassEx(&wcex);
    DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, exStyle = WS_EX_TOPMOST|WS_EX_TOOLWINDOW;
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);
    hwndForVst = CreateWindowEx(exStyle, wcex.lpszClassName, A2W(effname), style, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL);
    vsthost.GetAt(nEffect)->pEffect->user = hwndForVst;
    SetWindowLongPtr(hwndForVst, GWLP_USERDATA, (LONG_PTR)this);
    DWORD e = GetLastError();
    return 0;
}

int VstPlugin::ReleasePlugin()
{
    ShowPluginWindow(false);
    DestroyWindow(hwndForVst);
    hwndForVst = nullptr;
    vsthost.EffSuspend(nEffect);
    vsthost.EffStopProcess(nEffect);
    vsthost.EffMainsChanged(nEffect, false);
    //vsthost.EffClose(nEffect);
	vsthost.RemoveAll();
    return 0;
}

int VstPlugin::ShowPluginWindow(bool show)
{
    if (show)
    {
        if (!IsPluginWindowShown())
        {
            ShowWindow(hwndForVst, SW_SHOW);
            vsthost.EffEditOpen(nEffect, hwndForVst);
            ERect* peffrect = NULL;
            vsthost.EffEditGetRect(nEffect, &peffrect);
            if (peffrect)
                vsthost.OnSizeWindow(nEffect, peffrect->right - peffrect->left, peffrect->bottom - peffrect->top);
        }
    }
    else
    {
        if (IsPluginWindowShown())
        {
            vsthost.EffEditClose(nEffect);
            ShowWindow(hwndForVst, SW_HIDE);
        }
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
    ve.deltaFrames = 0;
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
    ve.type = kVstSysExType;
    ve.byteSize = sizeof(ve);
    ve.deltaFrames = 0;
    ve.flags = kVstMidiEventIsRealtime;
    ve.dumpBytes = length;
    ve.sysexDump = (char*)data;
    VstEvents ves{};
    ves.numEvents = 1;
    ves.events[0] = (VstEvent*)&ve;
	vsthost.EffProcessEvents(nEffect, &ves);
    return 0;
}

void VstPlugin::OnIdle()
{
    vsthost.EffIdle(nEffect);
    vsthost.EffEditIdle(nEffect);
}

//如果全部为0则返回true，否则为false
template<typename T>bool CheckAllZero(T** p, int ylength, int xlength)
{
    for (int j = 0; j < ylength; j++)
    {
        for (int i = 0; i < xlength; i++)
        {
            if (p[j][i] != 0)
                return false;
        }
    }
    return true;
}

int VstPlugin::ExportToWav(LPCTSTR midiFilePath, LPCTSTR wavFilePath)
{
    int oldChannelCount = player.GetChannelCount(), oldSampleRate = player.GetSampleRate();
    StopPlayback();
    //加载MIDI
    std::ifstream f(midiFilePath, std::ios::binary);
    std::stringstream fm;
    if (!f)
        return -1;
    fm << f.rdbuf();
    //提取RMI文件中的MIDI
    char magic[4];
    fm.read(magic, ARRAYSIZE(magic));
    if (strncmp(magic, "RIFF", ARRAYSIZE(magic)) == 0)
    {
        struct RMIDHeader
        {
            char strRIFF[4];
            int chunkSize;
            char strFormat[4];
            char strdata[4];
            int midiFileChunkSize;
        }rmi;
        fm.seekg(0);
        fm.read((char*)&rmi, sizeof rmi);
        std::string fmstr = fm.str();
        fmstr = fmstr.substr(sizeof rmi, rmi.midiFileChunkSize);
        fm.str(fmstr);
    }
    else
        fm.seekg(0);
    smf::MidiFile mf;
    if (!mf.read(fm))
        return -1;
    mf.joinTracks();

    const int bytesPerVar = sizeof(float);//指的是一个采样点的一个通道占字节数
    const int sampleRate = 48000;
    TCHAR wfPath[MAX_PATH];
    lstrcpy(wfPath, wavFilePath);
    CWaveFile waveFile;
    WAVEFORMATEX wfex = {
        WAVE_FORMAT_IEEE_FLOAT,//audioFormat
        (WORD)player.GetChannelCount(),//numChannels
        sampleRate,//sampleRate
        sampleRate* player.GetChannelCount() * bytesPerVar,//byteRate
        (WORD)(player.GetChannelCount() * bytesPerVar),//blockAlign
        (WORD)(bytesPerVar * 8),//bpsample
        (WORD)0//cbSize
    };
    if (FAILED(waveFile.Open(wfPath, &wfex, WAVEFILE_WRITE)))
        return -1;
    const size_t sizeofWaveFileHeader = 46;
    //修改VST设定为导出文件的配置
    vsthost.SetSampleRate(wfex.nSamplesPerSec);
    vsthost.SetBlockSize(1024);
    //先写入后面的WAV数据，并设定subchunk2Size为音频部分总字节数
    //格式：
    //--------采样1-------|--------采样2-------|…
    //通道1|通道2|通道3|…|通道1|通道2|通道3|…|…
    //准备时长为1(1/1)秒的缓冲区
    int allocChIn = max(wfex.nChannels, vsthost.GetAt(nEffect)->pEffect->numInputs);
    int allocChOut = max(wfex.nChannels, vsthost.GetAt(nEffect)->pEffect->numOutputs);
    float** wavBufferIn = new float* [allocChIn];
    float** wavBufferOut = new float* [allocChOut];
    int smpsPerBuffer = wfex.nSamplesPerSec;
    for (int i = 0; i < allocChIn; i++)
        wavBufferIn[i] = new float[smpsPerBuffer];
    for (int i = 0; i < allocChOut; i++)
        wavBufferOut[i] = new float[smpsPerBuffer];
    int cursorMidiEvents = 0, cursorSample = 0;
    double bpm = 120.0;
    int lastMidiEventSample = 0, lastMidiEventTick = 0;
    while (true)
    {
        //发送MIDI消息
        for (int i = 0; i < smpsPerBuffer; i++)
        {
            while (cursorMidiEvents < mf[0].size())
            {
                //需要将tick换算成sample
                int midiEventSample = lastMidiEventSample + (mf[0][cursorMidiEvents].tick - lastMidiEventTick) * 60 * sampleRate / mf.getTicksPerQuarterNote() / bpm;
                if (midiEventSample >= cursorSample + i)
                    break;
                if (mf[0][cursorMidiEvents].isTempo())
                    bpm = mf[0][cursorMidiEvents].getTempoBPM();
                if ((mf[0][cursorMidiEvents].data()[0] & 0xF0) == 0xF0)
                {
                    VstMidiSysexEvent vexs{};
                    vexs.type = kVstSysExType;
                    vexs.byteSize = sizeof(vexs);
                    vexs.deltaFrames = i;
                    vexs.flags = 0;
                    vexs.dumpBytes = mf[0][cursorMidiEvents].size();
                    vexs.sysexDump = (char*)mf[0][cursorMidiEvents].data();
                    VstEvents ves{};
                    ves.numEvents = 1;
                    ves.events[0] = (VstEvent*)&vexs;
                    vsthost.EffProcessEvents(nEffect, &ves);
                }
                else
                {
                    VstMidiEvent vms{};
                    vms.type = kVstMidiType;
                    vms.byteSize = sizeof(vms);
                    vms.deltaFrames = i;
                    vms.flags = 0;
                    memcpy(vms.midiData, mf[0][cursorMidiEvents].data(), sizeof(vms.midiData));
                    VstEvents ves{};
                    ves.numEvents = 1;
                    ves.events[0] = (VstEvent*)&vms;
                    vsthost.EffProcessEvents(nEffect, &ves);
                }
                lastMidiEventSample = midiEventSample;
                lastMidiEventTick = mf[0][cursorMidiEvents].tick;
                cursorMidiEvents++;
            }
        }
        cursorSample += smpsPerBuffer;
        //接收、写入音频
        for (int i = 0; i < allocChIn; i++)
            ZeroMemory(wavBufferIn[i], smpsPerBuffer * sizeof(float));
        for (int i = 0; i < allocChOut; i++)
            ZeroMemory(wavBufferOut[i], smpsPerBuffer * sizeof(float));
        if (vsthost.GetAt(nEffect)->pEffect->flags & effFlagsCanReplacing)
            vsthost.EffProcessReplacing(nEffect, wavBufferIn, wavBufferOut, smpsPerBuffer);
        else
            vsthost.EffProcess(nEffect, wavBufferIn, wavBufferOut, smpsPerBuffer);
        for (size_t i = 0; i < smpsPerBuffer; i++)
        {
            for (int j = 0; j < wfex.nChannels; j++)
            {
                UINT cbWrote;
                if (FAILED(waveFile.Write(sizeof(float), (PBYTE)&wavBufferOut[j][i], &cbWrote)))
                    return -1;
            }
        }
        //如果MIDI已结束且音频已静音则退出循环
        if (cursorMidiEvents >= mf[0].size() && CheckAllZero(wavBufferIn, wfex.nChannels, smpsPerBuffer))
            break;
    }
    //释放内存
    for (int i = 0; i < allocChIn; i++)
        delete[]wavBufferIn[i];
    for (int i = 0; i < allocChOut; i++)
        delete[]wavBufferOut[i];
    delete[]wavBufferIn;
    delete[]wavBufferOut;
    //恢复原来的设定
    vsthost.SetSampleRate(m_smpRate);
    vsthost.SetBlockSize(m_blockSize);
    StartPlayback(oldChannelCount, oldSampleRate);
    return 0;
}

void VstPlugin::_Playback()
{
    UINT buffer_time_ms = GetPrivateProfileInt(TEXT("XAudio2"), TEXT(KEYNAME_BUFFER_LENGTH), VDEFAULT_BUFFER_LENGTH, TEXT(".\\VisualMIDIPlayer.ini"));
    size_t bytesof_soundBuffer = player.GetSampleRate() * player.GetChannelCount() * player.GetBytesPerVar() * buffer_time_ms / 1000;
    ATLASSERT(player.GetBytesPerVar() == 2);//因为下面要用short类型，必须得确认这里是2
    short* buffer = reinterpret_cast<short*>(new BYTE[bytesof_soundBuffer]);

    size_t singleChSamples = player.GetSampleRate() * buffer_time_ms / 1000;
    std::vector<float*>pfChannelsIn,pfChannelsOut;
    long numInputs = max(player.GetChannelCount(), vsthost.GetAt(nEffect)->pEffect->numInputs);
    long numOutputs = max(player.GetChannelCount(), vsthost.GetAt(nEffect)->pEffect->numOutputs);//通道数必须至少为该参数的数值，否则极有可能报错
    for (int i = 0; i < numInputs; i++)
        pfChannelsIn.push_back(new float[singleChSamples]);
    for (int i = 0; i < numOutputs; i++)
        pfChannelsOut.push_back(new float[singleChSamples]);
    while (isPlayingBack)
    {
		for (int i = 0; i < numInputs; i++)
			ZeroMemory(pfChannelsIn[i], singleChSamples*sizeof(float));
		if (vsthost.GetAt(nEffect)->pEffect->flags&effFlagsCanReplacing)
			vsthost.EffProcessReplacing(nEffect, pfChannelsIn.data(), pfChannelsOut.data(), singleChSamples);
		else
			vsthost.EffProcess(nEffect, pfChannelsIn.data(), pfChannelsOut.data(), singleChSamples);
        for (size_t i = 0; i < singleChSamples; i++)
        {
            for (int j = 0; j < player.GetChannelCount(); j++)
            {
                //某些做得比较糙的插件会出现数值在[-1,1]范围以外的情况，注意限定范围
                buffer[player.GetChannelCount() * i + j] = (short)(min(max(-32768.f, pfChannelsOut[j][i] * 32767.f), 32767.f));
            }
        }
        player.Play((BYTE*)buffer, bytesof_soundBuffer);
        player.WaitForBufferEndEvent();
    }
    for (int i = 0; i < numInputs; i++)
        delete[]pfChannelsIn[i];
    for (int i = 0; i < numOutputs; i++)
        delete[]pfChannelsOut[i];
    delete[]buffer;
}
