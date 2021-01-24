// CVSTHost.cpp : Defines the exported functions for the DLL application.
//

#define _CRT_SECURE_NO_WARNINGS

#include "../../build/msvc/2019/header.h"
#include "../CVSTHost.h"

#include <string>
#include <assert.h>
//#include "unicodestuff.h"
#include "../../deps/VST2_SDK/pluginterfaces/vst2.x/aeffectx.h"

#include <algorithm>

#define PRODUCT_STRING "SOMEPRODUCT"
#define VENDOR_STRING "SOMEVENDOR"

#include "../CanDos.h"

#include "unicodestuff.h"

#define MAX_MIDI_EVENTS 4096 // far beyond what would ever normally appear in a single low-latency buffer (~256 samples or so)
struct MyVSTEvents { // redeclaration of VstEvents, to support our own max number of events [see constant above]
    VstInt32 numEvents;
    VstIntPtr reserved;
    VstEvent *events[MAX_MIDI_EVENTS];
};

static CVST_EventCallback apiClientCallback = nullptr;

struct _CVST_Plugin {
private:
    AEffect * effect = nullptr;
public:
    void *userData = nullptr;
    HMODULE libraryHandle = NULL;
    bool editorOpen = false;
    //bool loaded = false;
    bool isInstrument = false;

    // storage for the actual events
    VstMidiEvent midiEventStorage[MAX_MIDI_EVENTS];
    // structure that gets sent to plugin -- 
    //   we pre-initialize the .events ptr to always point to the contiguous midi events in the storage above
    MyVSTEvents vstEvents;

    bool wantsIdle = false;

    _CVST_Plugin(AEffect *effect) {
        this->effect = effect;
        effect->resvd1 = (VstIntPtr)this;

        // pre-init the event storage
        for (int i = 0; i < MAX_MIDI_EVENTS; i++) {
            midiEventStorage[i].type = kVstMidiType;
            midiEventStorage[i].byteSize = sizeof(VstMidiEvent);
            midiEventStorage[i].flags = kVstMidiEventIsRealtime;
            vstEvents.events[i] = (VstEvent *)&midiEventStorage[i];
        }
    }

    inline VstIntPtr dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) {
        return effect->dispatcher(effect, opcode, index, value, ptr, opt);
    }
    inline void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames) {
        if (effect->flags & effFlagsCanReplacing)
            effect->processReplacing(effect, inputs, outputs, sampleFrames);
        else
            effect->__processDeprecated(effect, inputs, outputs, sampleFrames);
    }
    inline void processDoubleReplacing(double** inputs, double** outputs, VstInt32 sampleFrames) {
        effect->processDoubleReplacing(effect, inputs, outputs, sampleFrames);
    }
    inline void setParameter(VstInt32 index, float parameter) {
        effect->setParameter(effect, index, parameter);
    }
    inline float getParameter(VstInt32 index) {
        return effect->getParameter(effect, index);
    }

    inline int getNumInputs() { return effect->numInputs; }
    inline int getNumOutputs() { return effect->numOutputs; }
};

void logMessage(const char *message) {
    CVST_HostEvent hostEvent;
    hostEvent.eventType = CVST_EventType_Log;
    hostEvent.logEvent.message = message;
    apiClientCallback(&hostEvent, nullptr, nullptr);
}

#define FORMAT_BUFFER_LEN 10*1024
char formatBuffer[FORMAT_BUFFER_LEN];
void logFormat(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf_s<FORMAT_BUFFER_LEN>(formatBuffer, format, args);
    va_end(args);
    //
    logMessage(formatBuffer);
}

static const char *vendor_str = "(VENDOR)";
static const char *product_str = "(PRODUCT)";
static int vendor_version = -1;

CVSTHOST_API void CDECL CVST_Init(CVST_EventCallback callback)
{
    apiClientCallback = callback;
    logMessage("Hello from CVST_Init"); // only after we set the callback :P

                                      // get some global info up front
    CVST_HostEvent hostEvent;
    hostEvent.handled = false;
    hostEvent.eventType = CVST_EventType_GetVendorInfo;
    apiClientCallback(&hostEvent, nullptr, nullptr);
    if (hostEvent.handled) {
        vendor_str = _strdup(hostEvent.vendorInfoEvent.vendor);
        product_str = _strdup(hostEvent.vendorInfoEvent.product);
        vendor_version = hostEvent.vendorInfoEvent.version;
        logFormat("got product info: %s: %s (ver %d)", vendor_str, product_str, vendor_version);
    }
}

CVSTHOST_API void CDECL CVST_Shutdown()
{
    logMessage("Goodbye from CVST_Shutdown");
    apiClientCallback = nullptr;
}

typedef AEffect *(*vstPluginFuncPtr)(audioMasterCallback host);

VstIntPtr VSTCALLBACK hostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
    CVST_Plugin plugin = effect ? (CVST_Plugin)effect->resvd1 : NULL;

    CVST_HostEvent hostEvent;
    hostEvent.handled = false;

    // a few messages can be handled with or without a valid plugin ptr
    // (notice these all return directly)
    switch (opcode) {
    case audioMasterVersion:
        return kVstVersion;

    case audioMasterGetCurrentProcessLevel:
        return kVstProcessLevelRealtime;

    case audioMasterGetVendorString:
        strncpy_s((char *)ptr, kVstMaxVendorStrLen, vendor_str, strlen(vendor_str)); // vendor_str and product_str are prefetched upon CVSTInit
        return true;

    case audioMasterGetProductString:
        strncpy_s((char *)ptr, kVstMaxProductStrLen, product_str, strlen(product_str));
        return true;

    case audioMasterGetVendorVersion:
        return vendor_version;

    case audioMasterUpdateDisplay:
        logMessage("audioMasterUpdateDisplay");
        return true;

    case audioMasterGetTime:
        return 0; // or VstTimeInfo*
    }

    // some plugins are jerks and send us things like automation messages before we even have an AEffect (from the first call to mainEntryPoint)
    // thus there won't be a CVstPlugin instance associated with it yet (gotten from ->resvd1 above)
    // hence the 'if' filter below ...
    if (plugin) {
        switch (opcode) {
        case audioMasterIdle:
            // not sure what to do here ...
            // discussion https://www.kvraudio.com/forum/viewtopic.php?t=349866
            break;
        case __audioMasterWantMidiDeprecated: // ??
            break;
        case __audioMasterNeedIdleDeprecated:
            plugin->wantsIdle = true; // send effIdle on idle pulse
            break;
        case audioMasterAutomate:
            //logFormat("automating param %d value %f", index, opt);
            hostEvent.eventType = CVST_EventType_Automation;
            hostEvent.automationEvent.index = index;
            hostEvent.automationEvent.value = opt;
            apiClientCallback(&hostEvent, plugin, plugin->userData);
            break;
        case audioMasterBeginEdit:
            logFormat("edit param %d BEGIN", index);
            break;
        case audioMasterEndEdit:
            logFormat("edit param %d END", index);
            break;
        case audioMasterIOChanged:
            logFormat("audioMasterIOChanged event");
            break;
        case audioMasterProcessEvents: {
            auto events = (VstEvents*)ptr;
            logFormat("host received %d VstEvents from plugin", events->numEvents);
            //for (int i = 0; i < events->numEvents; i++) {
            //    auto ev = events->events[i];
            //    logFormat(" = event %d type: %d", i, ev->type);
            //}
            break;
        }
        case audioMasterCanDo: {
            logFormat("audioMasterCanDo [%s]?", (const char*)ptr);
            return 0; // for now, until we handle these individually
        }
        case audioMasterSizeWindow:
            hostEvent.eventType = CVST_EventType_SizeWindow;
            hostEvent.sizeWindowEvent.width = index;
            hostEvent.sizeWindowEvent.height = value;
            return apiClientCallback(&hostEvent, plugin, plugin->userData);
        default:
            logFormat("unhandled vst host opcode (with plugin): %d", opcode);
            return false; // unhandled by default
        }
        return true; // unless otherwise specified
    }
    else {
        switch (opcode) {
            // these all definitely require a plugin instance, so silence them
        case audioMasterAutomate:
            break;
        default:
            logFormat("unhandled vst host opcode (null plugin): %d", opcode);
        }
    }
    return 0;
}

CVSTHOST_API CVST_Plugin CDECL CVST_LoadPlugin(const char *pathToPlugin, void *userData)
{
    logFormat("** loading [%s] **", pathToPlugin);
    auto widePath = utf8_to_wstring(pathToPlugin);
    auto libHandle = LoadLibraryW(widePath.c_str());
    if (libHandle == NULL) {
        logMessage("DLL not found / LoadLibrary failed");
        return NULL;
    }

    vstPluginFuncPtr mainEntryPoint;

    mainEntryPoint = (vstPluginFuncPtr)GetProcAddress(libHandle, "VSTPluginMain");
    if (!mainEntryPoint) {
        logMessage("'VSTPluginMain' entry point not found, trying 'main'");
        mainEntryPoint = (vstPluginFuncPtr)GetProcAddress(libHandle, "main");
        if (!mainEntryPoint) {
            logMessage("'main' entry point not found, either");
            return NULL;
        }
    }

    logFormat("main entry point: %08X", mainEntryPoint);

    auto effect = mainEntryPoint(hostCallback);
    logFormat("mplugin: %08X", effect);
    if (effect->magic == kEffectMagic) {
        auto ret = new _CVST_Plugin(effect);
        //effect->resvd1 = (VstIntPtr)ret; // now done in the constructor
        //ret->loaded = true;
        ret->userData = userData;
        ret->libraryHandle = libHandle;
        ret->editorOpen = false;
        ret->isInstrument = false;

        if (ret->dispatcher(effCanDo, 0, 0, (void *)PlugCanDos::canDoReceiveVstMidiEvent, 0.0f) == 1) {
            ret->isInstrument = true;
        }
        return ret;
    }
    else {
        logMessage("VST magic incorrect, unloading ...");
        return NULL;
    }
}

CVSTHOST_API void CDECL CVST_Destroy(CVST_Plugin plugin)
{
    if (plugin->libraryHandle) {
        plugin->dispatcher(effClose, 0, 0, NULL, 0.0f);
        FreeLibrary(plugin->libraryHandle);
        logFormat("library handle: %08X", plugin->libraryHandle);
        plugin->libraryHandle = NULL;
        logMessage(" ... freed library");
    }
    else {
        logMessage("library handle null? not freeing");
    }
    delete plugin;
}

CVSTHOST_API void CDECL CVST_Start(CVST_Plugin plugin, float sampleRate)
{
    plugin->dispatcher(effOpen, 0, 0, NULL, 0.0f);
    plugin->dispatcher(effSetSampleRate, 0, 0, NULL, sampleRate);
}

CVSTHOST_API void CDECL CVST_SetBlockSize(CVST_Plugin plugin, int blockSize)
{
    plugin->dispatcher(effSetBlockSize, 0, blockSize, NULL, 0.0f);
}

CVSTHOST_API void CDECL CVST_Suspend(CVST_Plugin plugin)
{
    plugin->dispatcher(effStopProcess, 0, 0, NULL, 0.0f);
    plugin->dispatcher(effMainsChanged, 0, 0, NULL, 0.0f);
}

CVSTHOST_API void CDECL CVST_Resume(CVST_Plugin plugin)
{
    plugin->dispatcher(effMainsChanged, 0, 1, NULL, 0.0f);
    plugin->dispatcher(effStartProcess, 0, 0, NULL, 0.0f);
}

CVSTHOST_API void CDECL CVST_GetEditorSize(CVST_Plugin plugin, int *width, int *height)
{
    ERect r;
    ERect* result = &r; // allocated by plugin ... who is responsible for freeing?
    plugin->dispatcher(effEditGetRect, 0, 0, &result, 0.0f);
    *width = result->right - result->left;
    *height = result->bottom - result->top;
}

CVSTHOST_API void CDECL CVST_GetEffectName(CVST_Plugin plugin, char* ptr)
{
    plugin->dispatcher(effGetEffectName, 0, 0, ptr, 0.0f);
}

CVSTHOST_API void CDECL CVST_OpenEditor(CVST_Plugin plugin, size_t windowHandle)
{
    if (!plugin->editorOpen) {
        logMessage("showing plugin window");
        plugin->dispatcher(effEditOpen, 0, 0, (void *)windowHandle, 0.0f);
        plugin->editorOpen = true;
    }
}

CVSTHOST_API void CDECL CVST_CloseEditor(CVST_Plugin plugin)
{
    if (plugin->editorOpen) {
        plugin->dispatcher(effEditClose, 0, 0, NULL, 0.0f);
        plugin->editorOpen = false;
    }
}

CVSTHOST_API void CDECL CVST_ProcessReplacing(CVST_Plugin plugin, float **inputs, float **outputs, unsigned int sampleFrames)
{
    // process audio
    plugin->processReplacing(inputs, outputs, sampleFrames);
}

CVSTHOST_API void CDECL CVST_Idle(CVST_Plugin plugin)
{
    if (plugin->wantsIdle) {
        plugin->dispatcher(__effIdleDeprecated, 0, 0, NULL, 0.0f);
    }
    if (plugin->editorOpen) {
        plugin->dispatcher(effEditIdle, 0, 0, NULL, 0.0f);
    }
}

CVSTHOST_API void CDECL CVST_SetBlockEvents(CVST_Plugin plugin, CVST_MidiEvent *events, int numEvents)
{
    // convert incoming events to what the VST wants
    if (numEvents > 0) {
        numEvents = min(numEvents, MAX_MIDI_EVENTS);
        size_t lastOffs = 0;
        for (int i = 0; i < numEvents; i++) {
            VstMidiEvent &vme = plugin->midiEventStorage[i];
            // other fields already set in CVstPlugin constructor
            vme.deltaFrames = (VstInt32)(events[i].sampleOffs - lastOffs);
            lastOffs = events[i].sampleOffs;
            //logFormat("lastOffs: %d", lastOffs);
            //
            *((UINT32 *)vme.midiData) = events[i].data.uint32; // copy all 4 bytes at once (even if only 3 are used)
        }
        plugin->vstEvents.numEvents = numEvents;
        plugin->dispatcher(effProcessEvents, 0, 0, &plugin->vstEvents, 0.0f);
    }
}

CVSTHOST_API void CDECL CVST_SendEvents(CVST_Plugin plugin, VstEvents* events)
{
    plugin->dispatcher(effProcessEvents, 0, 0, events, 0.0f);
}

CVSTHOST_API void CDECL CVST_GetProperties(CVST_Plugin plugin, CVST_Properties *props)
{
    props->numInputs = plugin->getNumInputs();
    props->numOutputs = plugin->getNumOutputs();
    props->isInstrument = plugin->isInstrument; // set on plugin load, so not a function
}

CVSTHOST_API void CDECL CVST_GetChunk(CVST_Plugin plugin, enum CVST_ChunkType chunkType, void** data, size_t* length)
{
    VstInt32 index = chunkType == ChunkType_Bank ? 0 : 1;
    *length = plugin->dispatcher(effGetChunk, index, 0, data, 0.0f);
    // will be up to the client to save the block elsewhere immediately
}

CVSTHOST_API void CDECL CVST_SetChunk(CVST_Plugin plugin, enum CVST_ChunkType chunkType, void* source, size_t length)
{
    VstInt32 index = chunkType == ChunkType_Bank ? 0 : 1;
    plugin->dispatcher(effSetChunk, index, length, source, 0.0f);
}
