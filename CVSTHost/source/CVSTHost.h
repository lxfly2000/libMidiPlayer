#ifndef __CVSTHOST_H__
#define __CVSTHOST_H__

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CVSTHOST_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CVSTHOST_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef CVSTHOST_EXPORTS
#define CVSTHOST_API //__declspec(dllexport)
#else
#define CVSTHOST_API //__declspec(dllimport)
#endif

#define CDECL // including Windows.h is a bit overkill just for this

#define APIHANDLE(x) struct _##x; typedef struct _##x* x

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include "../deps/VST2_SDK/pluginterfaces/vst2.x/aeffectx.h"

#ifdef __cplusplus
extern "C" {
#endif

    APIHANDLE(CVST_Plugin);

    typedef enum {
        CVST_EventType_Log,
        CVST_EventType_Automation,
        CVST_EventType_GetVendorInfo,
        CVST_EventType_SizeWindow
    } CVST_EventType;

    typedef struct {
        CVST_EventType eventType;
        bool handled;
        // union of stuff
        union {
            struct {
                const char *message;
            } logEvent;
            struct {
                int index;
                float value;
            } automationEvent;
            struct {
                const char *vendor;
                const char *product;
                int version;
            } vendorInfoEvent;
            struct {
                int width;
                int height;
            } sizeWindowEvent;
        };
    } CVST_HostEvent;

    typedef int(CDECL *CVST_EventCallback)(CVST_HostEvent *event, CVST_Plugin plugin, void *userData); // akin to win32 wndproc, handles everything

    CVSTHOST_API void CDECL CVST_Init(CVST_EventCallback callback);
    CVSTHOST_API void CDECL CVST_Shutdown();

    CVSTHOST_API CVST_Plugin CDECL CVST_LoadPlugin(const char *pathToPlugin, void *userData);
    CVSTHOST_API void CDECL CVST_Destroy(CVST_Plugin plugin);

    CVSTHOST_API void CDECL CVST_Start(CVST_Plugin plugin, float sampleRate);
    CVSTHOST_API void CDECL CVST_SetBlockSize(CVST_Plugin plugin, int blockSize);
    CVSTHOST_API void CDECL CVST_Resume(CVST_Plugin plugin);
    CVSTHOST_API void CDECL CVST_Suspend(CVST_Plugin plugin);
    CVSTHOST_API void CDECL CVST_GetEditorSize(CVST_Plugin plugin, int *width, int *height);
    CVSTHOST_API void CDECL CVST_GetEffectName(CVST_Plugin plugin, char *ptr);
    CVSTHOST_API void CDECL CVST_OpenEditor(CVST_Plugin plugin, size_t windowHandle);
    CVSTHOST_API void CDECL CVST_CloseEditor(CVST_Plugin plugin);
    CVSTHOST_API void CDECL CVST_ProcessReplacing(CVST_Plugin plugin, float **inputs, float **outputs, unsigned int sampleFrames);
    CVSTHOST_API void CDECL CVST_Idle(CVST_Plugin plugin);

    typedef struct {
        unsigned long sampleOffs; // relative to start of block -- NOT deltas, that's calculated in CVSTSetBlockEvents
        union {
            unsigned char bytes[4];
            unsigned int uint32;
        } data;
    } CVST_MidiEvent;
    CVSTHOST_API void CDECL CVST_SetBlockEvents(CVST_Plugin plugin, CVST_MidiEvent *events, int numEvents);
    CVSTHOST_API void CDECL CVST_SendEvents(CVST_Plugin plugin, VstEvents* events);

    typedef struct {
        int numInputs;
        int numOutputs;
        bool isInstrument;
    } CVST_Properties;
    CVSTHOST_API void CDECL CVST_GetProperties(CVST_Plugin plugin, CVST_Properties *props);

    enum CVST_ChunkType {
        ChunkType_Bank,
        ChunkType_Program
    };
    CVSTHOST_API void CDECL CVST_GetChunk(CVST_Plugin plugin, enum CVST_ChunkType chunkType, void** data, size_t* length); // will allocate and copy
    CVSTHOST_API void CDECL CVST_SetChunk(CVST_Plugin plugin, enum CVST_ChunkType chunkType, void* source, size_t length); // set from memory

#ifdef __cplusplus
}
#endif



#endif // __CVSTHOST_H__
