#include "directory_watcher.h"

// TODO: is this not needed? g++ complains about redefinition...
//#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_STRING_AREA_SIZE (128 * 1024)
#define BUFFER_SIZE (sizeof(DirWatcherEvent) + BUFFER_STRING_AREA_SIZE)

// TODO: exactly this should probably be configurable on DirWatcher init.
// Or rather: be able to define multiple reactions, each with their own filename/extention conditions
static bool _shouldTriggerBuild(const char* fileName, const char* extension) {
    if (extension != NULL && strcmp(extension, "cpp") == 0) {
        return true;
    }
    if (extension != NULL && strcmp(extension, "h") == 0) {
        return true;
    }
    if (fileName != NULL && strcmp(fileName, "Makefile") == 0) {
        return true;
    }

    return false;
}

// NOTE: This function runs in a child process.
// It gets passed the DirWatcherState (which is copied when forking).
// Also the allocated buffer inside DirWatcherState is automatically copied when forking.
static int _DirWatcher_ChildProc(void* arg) {
    // This ensures the child proc is also terminated if the parent crashes or exits.
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    int notifyFd = inotify_init1(IN_NONBLOCK);
    if (notifyFd == -1) {
        perror("_DirWatcher_ChildProc  inotify_init1");
        return 1;
    }

    DirWatcherState* state = (DirWatcherState*)arg;
    close(state->pipe.readFd);
    state->pipe.readFd = 0;

    int wds[DIRWATCHER_MAX_DIRECTORIES];
    for (int i = 0; i < state->config.directoryCount; ++i) {
        const char* path = state->config.directories[i];
        wds[i] = inotify_add_watch(notifyFd, path, IN_CLOSE_WRITE);
        if (wds[i] == -1) {
            fprintf(stderr, "[DirWatcher] Cannot watch '%s': %s\n", path, strerror(errno));
        }
        else {
            printf("[DirWatcher] Watching '%s'\n", path);
        }
    }

    char eventBuffer[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    while (true) {
        //usleep(1000);
        //printf("child reading notify\n");
        ssize_t size = read(notifyFd, eventBuffer, sizeof(eventBuffer));
        if (size == -1 && errno != EAGAIN) {
            perror("_DirWatcher_ChildProc  read notifyFd");
            return 1;
        }

        if (size <= 0) {
            continue;
        }

        memset(state->buffer, 0, BUFFER_SIZE);
        DirWatcherEvent* watcherEvent = (DirWatcherEvent*)state->buffer;
        watcherEvent->type = DIRWATCHER_EVENT_FILE_CHANGED;

        bool triggerBuild = false;

        // NOTE:FileChanged event passes a list of char*
        // we grow the list from the start of the state buffer
        // and the actual strings from the end of the buffer
        // then we collapse the empty space in the middle
        // and re-calculate the ptrs to relative addressing
        // which is then resolved in PollEvent
        const char** firstListEntry = (const char**)(state->buffer + sizeof(DirWatcherEvent));
        watcherEvent->fileChanged.paths = firstListEntry;
        char* top = state->buffer + BUFFER_SIZE;
        char path[PATH_MAX];
        for (ssize_t i = 0; i < size; i += sizeof(struct inotify_event)) {
            struct inotify_event* notifyEvent = (struct inotify_event*)(eventBuffer + i);
            if (notifyEvent->mask & IN_CLOSE_WRITE) {
                int directoryIndex = -1;
                for (int j = 0; j < state->config.directoryCount; ++j) {
                    if (wds[j] == notifyEvent->wd) {
                        directoryIndex = j;
                        break;
                    }
                }

                if (notifyEvent->len > 0) {
                    char* pathPtr = notifyEvent->name;
                    if (directoryIndex >= 0) {
                        snprintf(path, sizeof(path), "%s/%s", state->config.directories[directoryIndex], notifyEvent->name);
                        pathPtr = path;
                    }

                    printf("[DirWatcher] File '%s' changed.\n", pathPtr);

                    size_t len = getStringLength(pathPtr) + 1;
                    // TODO: gracefully handle us not having enough space!
                    assert((intptr_t)(top - len) > (intptr_t)(watcherEvent->fileChanged.paths + watcherEvent->fileChanged.count + 1));
                    top -= len;
                    memcpy(top, pathPtr, len);

                    watcherEvent->fileChanged.paths[watcherEvent->fileChanged.count] = top;
                    ++watcherEvent->fileChanged.count;

                    char* fileName = splitAtLastOccurence(pathPtr, '/');
                    const char* extension = splitAtLastOccurence(fileName, '.');
                    if (_shouldTriggerBuild(fileName, extension)) {
                        triggerBuild = true;
                    }
                }
            }

            i += notifyEvent->len;
        }

        // Collapse empty space
        char* afterLastListEntry = (char*)(watcherEvent->fileChanged.paths + watcherEvent->fileChanged.count);
        int totalStringSize = (int)(state->buffer + BUFFER_SIZE - top);
        memmove(afterLastListEntry, top, totalStringSize);

        // Calculate relative addresses for the string pointers
        intptr_t afterLastListEntryOffset = (intptr_t)afterLastListEntry - (intptr_t)watcherEvent;
        for (int i = 0; i < watcherEvent->fileChanged.count; ++i) {
            const char* ptr = watcherEvent->fileChanged.paths[i];
            intptr_t offsetFromTop = (intptr_t)ptr - (intptr_t)top;
            watcherEvent->fileChanged.paths[i] = (const char*)(afterLastListEntryOffset + offsetFromTop);
        }

        int totalEventSize = sizeof(DirWatcherEvent) + sizeof(char*) * watcherEvent->fileChanged.count + totalStringSize;
        write(state->pipe.writeFd, state->buffer, totalEventSize);

        if (triggerBuild) {
            memset(state->buffer, 0, BUFFER_SIZE);

            DirWatcherEvent* watcherEvent = (DirWatcherEvent*)state->buffer;
            watcherEvent->type = DIRWATCHER_EVENT_REACTION_REPORT;
            char* stringBuffer = state->buffer + sizeof(DirWatcherEvent);
            watcherEvent->reactionReport.statusMsg = (const char*)sizeof(DirWatcherEvent);

            // TODO: could this pipe could also be created once at initialization?
            Pipe p;
            int pipeRet = pipe((int*)&p);
            if (pipeRet == -1) {
                perror("_DirWatcher_ChildProc  pipe");
                continue;
            }

            int stdoutFd = dup(STDOUT_FILENO);
            int stderrFd = dup(STDERR_FILENO);
            // redirect both stdout and stderr to the read end of the pipe
            dup2(p.writeFd, STDOUT_FILENO);
            dup2(p.writeFd, STDERR_FILENO);
            // now we should be able to read from the pipe and get both stdout and stderr
            watcherEvent->reactionReport.exitStatus = system("make app");

            ssize_t size = read(p.readFd, stringBuffer, BUFFER_STRING_AREA_SIZE);
            if (size == -1) {
                perror("_DirWatcher_ChildProc  read pipeReadFd");
                continue;
            }

            stringBuffer[size] = '\0';
            ++size;

            // restore stdout and stderr
            dup2(stdoutFd, STDOUT_FILENO);
            dup2(stderrFd, STDERR_FILENO);

            close(p.readFd);
            close(p.writeFd);

            // NOTE: Don't leak file descriptors!
            close(stdoutFd);
            close(stderrFd);

            write(state->pipe.writeFd, state->buffer, sizeof(DirWatcherEvent) + size);
        }
    }

    // TODO: Is inotify cleanup required?

    printf("[DirWatcher] ChildProc shutting down.\n");
    close(state->pipe.writeFd);

    return 0;
}

void DirWatcherConfig_AppendDirectory(DirWatcherConfig* config, const char* path) {
    if (config->directoryCount < DIRWATCHER_MAX_DIRECTORIES) {
        // NOTE: we are leaking here, but I really don't care, as we want these paths to stick around for the whole program duration.
        config->directories[config->directoryCount] = strdup(path);
        ++config->directoryCount;
    }
}

bool DirWatcher_Init(DirWatcherState* state) {
    if (pipe2((int*)&state->pipe, O_DIRECT | O_NONBLOCK) != 0) {
        perror("DirWatcher_Init  pipe2");
        return false;
    }

    state->buffer = (char*)malloc(sizeof(DirWatcherEvent) + BUFFER_STRING_AREA_SIZE);
    assert(state->buffer != NULL);

    // TODO: Is the stack size appropriate? or should it be bigger?
#define STACK_SIZE (1024 * 1024)
    char* stack = (char*)malloc(STACK_SIZE);
    char* stackTop = stack + STACK_SIZE;
    state->childPid = clone(_DirWatcher_ChildProc, stackTop,
        CLONE_CHILD_CLEARTID | CLONE_CHILD_SETTID | SIGCHLD, state);
    if (state->childPid == -1) {
        perror("DirWatcher_Init  clone");
        close(state->pipe.readFd);
        close(state->pipe.writeFd);
        state->pipe.readFd = 0;
        state->pipe.writeFd = 0;

        free(state->buffer);
        state->buffer = NULL;
        return false;
    }

    state->initialized = true;
    close(state->pipe.writeFd);
    state->pipe.writeFd = 0;

    return true;
}

void DirWatcher_Shutdown(DirWatcherState* state) {
    state->initialized = false;
    state->pipeClosed = false;

    kill(state->childPid, SIGTERM);
    state->childPid = 0;

    close(state->pipe.readFd);
    state->pipe.readFd = 0;
    state->pipe.writeFd = 0;

    free(state->buffer);
    state->buffer = NULL;
}

// NOTE: This is called on the parent process.
bool DirWatcher_PollEvent(DirWatcherState* state, DirWatcherEvent* event) {
    if (!state->initialized) {
        return false;
    }
    if (state->pipeClosed) {
        return false;
    }

    ssize_t size = read(state->pipe.readFd, state->buffer, BUFFER_SIZE);
    if (size > 0) {
        memcpy(event, state->buffer, sizeof(DirWatcherEvent));

        // NOTE: Here we resolve the relative addresses of our string pointers to actual adresses again.
        switch (event->type) {
        case DIRWATCHER_EVENT_FILE_CHANGED:
        {
            // The list of string pointers is located right after the event
            // and the actual strings are located right after the list
            for (int i = 0; i < event->fileChanged.count; ++i) {
                event->fileChanged.paths[i] = (const char*)(state->buffer + (intptr_t)event->fileChanged.paths[i]);
            }
        } break;
        case DIRWATCHER_EVENT_REACTION_REPORT:
        {
            event->reactionReport.statusMsg = (const char*)(state->buffer + sizeof(DirWatcherEvent));
        } break;
        }

        return true;
    }

    if (size == 0) {
        state->pipeClosed = true;
        return false;
    }

    // size < 0
    if (errno == EAGAIN) {
        return false;
    }

    state->pipeClosed = true;
    perror("DirWatcher_PollEvent pipe read failed");

    return false;
}


