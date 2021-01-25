//���˼������ޣ��ù��ܶԲ���VST���֧��Ч�����ã����������MIDI�ӳ١����ֲ����������

#define _CRT_SECURE_NO_WARNINGS


#include "vstplugin.h"
#include <sstream>
#include <atlconv.h>
#include <MidiFile.h>

#include "SDKWaveFile.h"
#include "CVSTHost/source/CVSTHost.h"

#define KEYNAME_BUFFER_LENGTH	"BufferTime"
#define VDEFAULT_BUFFER_LENGTH	20



static CVST_Plugin g_plugin = nullptr;
static HWND hwndForVst = nullptr;
static bool editorOpened = false;
static CVST_Properties props;

bool OnSizeWindow(long width, long height)
{
    RECT rect{ 0,0,width,height };
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, NULL);
    SetWindowPos(hwndForVst, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
    return true;
}

int CDECL vstHostCallback(CVST_HostEvent* event, CVST_Plugin plugin, void* userData)
{
    event->handled = true;
    switch (event->eventType) {
    case CVST_EventType_Log:
        printf("VST>> %s\n", event->logEvent.message);
        break;
    case CVST_EventType_Automation:
        printf(" = vst automation [%03d] value %.2f\n", event->automationEvent.index, event->automationEvent.value);
        break;
    case CVST_EventType_GetVendorInfo:
        event->vendorInfoEvent.vendor = "Derp";
        event->vendorInfoEvent.product = "LibraryTest";
        event->vendorInfoEvent.version = 1234;
        break;
    case CVST_EventType_SizeWindow:
        return OnSizeWindow(event->sizeWindowEvent.width, event->sizeWindowEvent.height);
    default:
        event->handled = false;
    }
    return 0;
}


#define ALWAYS_ON_TOP 1

int VstPlugin::LoadPlugin(LPCTSTR path,int smpRate)
{
    USES_CONVERSION;
    CVST_Init(vstHostCallback);
    g_plugin = CVST_LoadPlugin(T2A(path), nullptr);
	if (!g_plugin)
		return -1;

    CVST_GetProperties(g_plugin, &props);
    CVST_Start(g_plugin, smpRate);

    m_smpRate = smpRate;
    CVST_SetBlockSize(g_plugin, m_blockSize = 512);

    CVST_Resume(g_plugin);

    char effname[256];
    int effwidth, effheight;
    CVST_GetEditorSize(g_plugin, &effwidth, &effheight);
    CVST_GetEffectName(g_plugin, effname);
    RECT rect{ 0,0,effwidth,effheight };
    WNDCLASSEX wcex{ sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW ,[](HWND hwnd,UINT msg,WPARAM w,LPARAM l)
        {
            switch (msg)
            {
            case WM_CREATE:
                AppendMenu(GetSystemMenu(hwnd, FALSE), MF_STRING | MF_CHECKED, ALWAYS_ON_TOP, TEXT("�ö���ʾ(&A)"));
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
                    ModifyMenu(GetSystemMenu(hwnd, FALSE), ALWAYS_ON_TOP, flags, ALWAYS_ON_TOP, TEXT("�ö���ʾ(&A)"));
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
    SetWindowLongPtr(hwndForVst, GWLP_USERDATA, (LONG_PTR)this);

    buffer_time_ms = GetPrivateProfileInt(TEXT("XAudio2"), TEXT(KEYNAME_BUFFER_LENGTH), VDEFAULT_BUFFER_LENGTH, TEXT(".\\VisualMIDIPlayer.ini"));
    bytesof_soundBuffer = player.GetSampleRate() * player.GetChannelCount() * player.GetBytesPerVar() * buffer_time_ms / 1000;
    ATLASSERT(player.GetBytesPerVar() == 2);//��Ϊ����Ҫ��short���ͣ������ȷ��������2
    buffer = reinterpret_cast<short*>(new BYTE[bytesof_soundBuffer]);

    singleChSamples = player.GetSampleRate() * buffer_time_ms / 1000;
    numInputs = max(player.GetChannelCount(), props.numInputs);
    numOutputs = max(player.GetChannelCount(), props.numOutputs);//ͨ������������Ϊ�ò�������ֵ�������п��ܱ���
    for (int i = 0; i < numInputs; i++)
        pfChannelsIn.push_back(new float[singleChSamples]);
    for (int i = 0; i < numOutputs; i++)
        pfChannelsOut.push_back(new float[singleChSamples]);
    return 0;
}

int VstPlugin::ReleasePlugin()
{
    for (int i = 0; i < numInputs; i++)
        delete[]pfChannelsIn[i];
    for (int i = 0; i < numOutputs; i++)
        delete[]pfChannelsOut[i];
    delete[]buffer;
    ShowPluginWindow(false);
    DestroyWindow(hwndForVst);
    hwndForVst = nullptr;
    CVST_Suspend(g_plugin);
    CVST_Destroy(g_plugin);
    CVST_Shutdown();
    return 0;
}

int VstPlugin::ShowPluginWindow(bool show)
{
    if (show)
    {
        if (!IsPluginWindowShown())
        {
            ShowWindow(hwndForVst, SW_SHOW);
            CVST_OpenEditor(g_plugin, (size_t)hwndForVst);
            int effwidth, effheight;
            CVST_GetEditorSize(g_plugin, &effwidth, &effheight);
            OnSizeWindow(effwidth, effheight);
            editorOpened = true;
        }
    }
    else
    {
        if (IsPluginWindowShown())
        {
            CVST_CloseEditor(g_plugin);
            ShowWindow(hwndForVst, SW_HIDE);
            editorOpened = false;
        }
    }
    return 0;
}

bool VstPlugin::IsPluginWindowShown()
{
    return editorOpened;
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
    //���ر�ע�⡿����SDK�ĵ�������processReplacingǰ���ֻ�ܵ���һ��processEvent��
    CVST_SendEvents(g_plugin, &ves);
    CVST_ProcessReplacing(g_plugin, pfChannelsIn.data(), pfChannelsOut.data(), 0);
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
    CVST_SendEvents(g_plugin, &ves);
    CVST_ProcessReplacing(g_plugin, pfChannelsIn.data(), pfChannelsOut.data(), 0);
    return 0;
}

void VstPlugin::OnIdle()
{
    CVST_Idle(g_plugin);
}

//���ȫ��Ϊ0�򷵻�true������Ϊfalse
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
    StopPlayback();
    //����MIDI
    std::ifstream f(midiFilePath, std::ios::binary);
    std::stringstream fm;
    if (!f)
        return -1;
    fm << f.rdbuf();
    //��ȡRMI�ļ��е�MIDI
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

    const int bytesPerVar = sizeof(float);//ָ����һ���������һ��ͨ��ռ�ֽ���
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
    //�޸�VST�趨Ϊ�����ļ�������
    CVST_Start(g_plugin, wfex.nSamplesPerSec);
    CVST_SetBlockSize(g_plugin, 1024);
    //��д������WAV���ݣ����趨subchunk2SizeΪ��Ƶ�������ֽ���
    //��ʽ��
    //--------����1-------|--------����2-------|��
    //ͨ��1|ͨ��2|ͨ��3|��|ͨ��1|ͨ��2|ͨ��3|��|��
    //׼��ʱ��Ϊ1(1/1)��Ļ�����
    int allocChIn = max(wfex.nChannels, props.numInputs);
    int allocChOut = max(wfex.nChannels, props.numOutputs);
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
        //����MIDI��Ϣ
        for (int i = 0; i < smpsPerBuffer; i++)
        {
            while (cursorMidiEvents < mf[0].size())
            {
                //��Ҫ��tick�����sample
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
                    CVST_SendEvents(g_plugin, &ves);
                    CVST_ProcessReplacing(g_plugin, wavBufferIn, wavBufferOut, 0);
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
                    CVST_SendEvents(g_plugin, &ves);
                    CVST_ProcessReplacing(g_plugin, wavBufferIn, wavBufferOut, 0);
                }
                lastMidiEventSample = midiEventSample;
                lastMidiEventTick = mf[0][cursorMidiEvents].tick;
                cursorMidiEvents++;
            }
        }
        cursorSample += smpsPerBuffer;
        //���ա�д����Ƶ
        for (int i = 0; i < allocChIn; i++)
            ZeroMemory(wavBufferIn[i], smpsPerBuffer * sizeof(float));
        for (int i = 0; i < allocChOut; i++)
            ZeroMemory(wavBufferOut[i], smpsPerBuffer * sizeof(float));
        CVST_ProcessReplacing(g_plugin, wavBufferIn, wavBufferOut, smpsPerBuffer);
        for (size_t i = 0; i < smpsPerBuffer; i++)
        {
            for (int j = 0; j < wfex.nChannels; j++)
            {
                UINT cbWrote;
                if (FAILED(waveFile.Write(sizeof(float), (PBYTE)&wavBufferOut[j][i], &cbWrote)))
                    return -1;
            }
        }
        //���MIDI�ѽ�������Ƶ�Ѿ������˳�ѭ��
        if (cursorMidiEvents >= mf[0].size() && CheckAllZero(wavBufferIn, wfex.nChannels, smpsPerBuffer))
            break;
    }
    //�ͷ��ڴ�
    for (int i = 0; i < allocChIn; i++)
        delete[]wavBufferIn[i];
    for (int i = 0; i < allocChOut; i++)
        delete[]wavBufferOut[i];
    delete[]wavBufferIn;
    delete[]wavBufferOut;
    //�ָ�ԭ�����趨
    CVST_Start(g_plugin,m_smpRate);
    CVST_SetBlockSize(g_plugin, m_blockSize);
    StartPlayback();
    return 0;
}

void VstPlugin::_Playback()
{
    while (isPlayingBack)
    {
		for (int i = 0; i < numInputs; i++)
			ZeroMemory(pfChannelsIn[i], singleChSamples*sizeof(float));
        CVST_ProcessReplacing(g_plugin, pfChannelsIn.data(), pfChannelsOut.data(), singleChSamples);
        for (size_t i = 0; i < singleChSamples; i++)
        {
            for (int j = 0; j < player.GetChannelCount(); j++)
            {
                //ĳЩ���ñȽϲڵĲ���������ֵ��[-1,1]��Χ����������ע���޶���Χ
                buffer[player.GetChannelCount() * i + j] = (short)(min(max(-32768.f, pfChannelsOut[j][i] * 32767.f), 32767.f));
            }
        }
        player.Play((BYTE*)buffer, bytesof_soundBuffer);
        player.WaitForBufferEndEvent();
    }
}
