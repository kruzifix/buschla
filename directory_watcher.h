#pragma once

#include "util.h"

#include <stdint.h>

// NOTE: This limitation is arbitrary, I'm just too lazy to implement a dynamic array.
#define DIRWATCHER_MAX_DIRECTORIES 256

typedef enum DirWatcherEventType {
    DIRWATCHER_EVENT_NONE = 0,

    DIRWATCHER_EVENT_FILE_CHANGED = 0x100,

    DIRWATCHER_EVENT_REACTION_REPORT = 0x200,
} DirWatcherEventType;

typedef struct {
    const char* directories[DIRWATCHER_MAX_DIRECTORIES];
    int directoryCount;
} DirWatcherConfig;

typedef struct {
    DirWatcherConfig config;

    bool initialized;
    // TODO: field to query if child proc is still running or crashed.

    int childPid;
    bool pipeClosed;
    Pipe pipe;

    char* buffer;
} DirWatcherState;

typedef struct {
    DirWatcherEventType type;
} DirWatcherEventCommon;

typedef struct {
    DirWatcherEventType type;
    int count;
    const char** paths;
} DirWatcherEventFileChanged;

typedef struct {
    DirWatcherEventType type;
    int exitStatus;
    const char* statusMsg;
} DirWatcherEventReactionReport;

typedef union {
    uint32_t type;
    DirWatcherEventCommon common;
    DirWatcherEventFileChanged fileChanged;
    DirWatcherEventReactionReport reactionReport;
} DirWatcherEvent;

void DirWatcherConfig_AppendDirectory(DirWatcherConfig* config, const char* path);

bool DirWatcher_Init(DirWatcherState* state);
void DirWatcher_Shutdown(DirWatcherState* state);

// NOTE: Any strings pointed to by the events are located in the DirWatcherState
// and are potentially overriden on the next call to DirWatcher_PollEvent!
bool DirWatcher_PollEvent(DirWatcherState* state, DirWatcherEvent* event);

