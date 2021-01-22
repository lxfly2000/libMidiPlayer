#define _CRT_SECURE_NO_WARNINGS


#include "vstplugin.h"
#include <vector>

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


    ShowPluginWindow(false);
    return 0;
}

int VstPlugin::ReleasePlugin()
{
	vsthost.RemoveAll();
    return 0;
}

int VstPlugin::ShowPluginWindow(bool show)
{
	if (show)
		vsthost.EffEditOpen(nEffect, nullptr);
	else
		vsthost.EffEditClose(nEffect);
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
    for (int i = 0; i < player.GetChannelCount(); i++)
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
    for (int i = 0; i < player.GetChannelCount(); i++)
        delete[]pfChannels[i];
    delete[]buffer;
}
