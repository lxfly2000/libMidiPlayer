#define _CRT_SECURE_NO_WARNINGS
#include "vstplugin.h"
#include "aeffect.h"
#include "aeffectx.h"
#include <vector>

#define KEYNAME_BUFFER_LENGTH	"BufferTime"
#define VDEFAULT_BUFFER_LENGTH	20



//https://blog.csdn.net/neo_yin/article/details/7354018
extern "C"{
// Main host callback
VstIntPtr VSTCALLBACK hostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstInt32 value, void* ptr, float opt)
{
    switch (opcode) {
    case audioMasterVersion:
        return 2400;
    case audioMasterIdle:
        effect->dispatcher(effect, effEditIdle, 0, 0, 0, 0);
        // Handle other opcodes here... there will be lots of them
    default:
        //DbgPrintfA("Plugin requested value of opcode %d\n", opcode);
        break;
    }
    return 0;
}
}


// Plugin's entry point
typedef AEffect* (*vstPluginFuncPtr)(audioMasterCallback host);
// Plugin's dispatcher function
typedef VstIntPtr(*dispatcherFuncPtr)(AEffect* effect, VstInt32 opCode, VstInt32 index, VstInt32 value, void* ptr, float opt);
// Plugin's getParameter() method
typedef float (*getParameterFuncPtr)(AEffect* effect, VstInt32 index);
// Plugin's setParameter() method
typedef void (*setParameterFuncPtr)(AEffect* effect, VstInt32 index, float value);
// Plugin's processEvents() method
typedef VstInt32(*processEventsFuncPtr)(VstEvents* events);
// Plugin's process() method
typedef void (*processFuncPtr)(AEffect* effect, float** inputs, float** outputs, VstInt32 sampleFrames);

static HMODULE g_modulePtr;
static AEffect* g_plugin;
static dispatcherFuncPtr g_dispatcher;


int VstPlugin::LoadPlugin(LPCTSTR path,int smpRate)
{
    g_modulePtr = LoadLibrary(path);
    if (g_modulePtr == NULL)
        return -1;

    vstPluginFuncPtr mainEntryPoint = (vstPluginFuncPtr)GetProcAddress(g_modulePtr, "VSTPluginMain");
    // Instantiate the plugin
    g_plugin = mainEntryPoint(hostCallback);

    // Check plugin's magic number
    // If incorrect, then the file either was not loaded properly, is not a real
    // VST plugin, or is otherwise corrupt.
    if (g_plugin->magic != kEffectMagic)
        return -2;

    // Create dispatcher handle
    g_dispatcher = (dispatcherFuncPtr)g_plugin->dispatcher;

    // Set up plugin callback functions
    /*plugin->getParameter = getParameter;
    plugin->processReplacing = processReplacing;
    plugin->setParameter = setParameter;*/

    g_dispatcher(g_plugin, effOpen, 0, 0, NULL, 0.0f);

    // Set some default properties
    g_dispatcher(g_plugin, effSetSampleRate, 0, 0, NULL, (float)smpRate);
    int blocksize = 512;
    g_dispatcher(g_plugin, effSetBlockSize, 0, blocksize, NULL, 0.0f);

    // Resume plugin
    g_dispatcher(g_plugin, effMainsChanged, 0, 1, NULL, 0.0f);


    ShowPluginWindow(false);
    return 0;
}

int VstPlugin::ReleasePlugin()
{
    FreeLibrary(g_modulePtr);
    return 0;
}

int VstPlugin::ShowPluginWindow(bool show)
{
    isPluginWindowShown = show;
    if (isPluginWindowShown)
        g_dispatcher(g_plugin, effEditOpen, 0, 0, 0, 0);
    else
        g_dispatcher(g_plugin, effEditClose, 0, 0, 0, 0);
    return 0;
}

bool VstPlugin::IsPluginWindowShown()
{
    return isPluginWindowShown;
}

void processMidi(AEffect* plugin, VstEvents* events) {
    g_dispatcher(plugin, effProcessEvents, 0, 0, events, 0.0f);
}

void processAudio(AEffect* plugin, float** inputs, float** outputs, long numFrames) {
    // Note: If you are processing an instrument, you should probably zero out the input
    // channels first to avoid any accidental noise. If you are processing an effect, you
    // should probably zero the values in the output channels. See the silenceChannel()
    // method below.
    plugin->processReplacing(plugin, inputs, outputs, numFrames);
}

void silenceChannel(float** channelData, int numChannels, long numFrames) {
    for (int channel = 0; channel < numChannels; ++channel) {
        for (long frame = 0; frame < numFrames; ++frame) {
            channelData[channel][frame] = 0.0f;
        }
    }
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
    processMidi(g_plugin, &ves);
    return 0;
}

int VstPlugin::SendSysExData(LPVOID data, DWORD length)
{
    VstMidiSysexEvent ve{};
    ve.type = kVstMidiType;
    ve.byteSize = sizeof(ve);
    ve.deltaFrames = 0;
    ve.flags = kVstMidiEventIsRealtime;
    ve.dumpBytes = length;
    memcpy(ve.sysexDump, data, length);
    VstEvents ves{};
    ves.numEvents = 1;
    ves.events[0] = (VstEvent*)&ve;
    processMidi(g_plugin, &ves);
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
        silenceChannel(pfChannels.data(), player.GetChannelCount(), singleChSamples);
        processAudio(g_plugin, pfChannels.data(), pfChannels.data(), singleChSamples);
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
