#if !defined(_WIN32) && !defined(__APPLE__)
#define _POSIX_C_SOURCE 200809L /* isatty, fileno, readlink, popen */
#endif

#include "terminal.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <stdint.h>
#endif

#ifdef _WIN32
static HANDLE console = INVALID_HANDLE_VALUE;
static DWORD saved_mode;
static UINT saved_codepage;

static void restore_console(void)
{
    fflush(stdout);
    SetConsoleMode(console, saved_mode);
    SetConsoleOutputCP(saved_codepage);
}
#endif

int myna_terminal_init(void)
{
    const char *no_color = getenv("NO_COLOR");
    const char *term = getenv("TERM");

    if ((no_color && *no_color) || (term && strcmp(term, "dumb") == 0))
        return 0;

#ifdef _WIN32
    if (!_isatty(_fileno(stdout)))
        return 0;
    console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console == INVALID_HANDLE_VALUE || !GetConsoleMode(console, &saved_mode))
        return 0;
    if (!SetConsoleMode(console, saved_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        return 0;
    saved_codepage = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
    atexit(restore_console);
    return 1;
#else
    return isatty(fileno(stdout));
#endif
}

void myna_terminal_set_title(const char *title)
{
    printf("\033]0;%s\007", title);
}

int myna_executable_path(char *buf, size_t size)
{
#if defined(_WIN32)
    DWORD len = GetModuleFileNameA(NULL, buf, (DWORD)size);
    return len > 0 && len < size;
#elif defined(__APPLE__)
    uint32_t len = (uint32_t)size;
    return _NSGetExecutablePath(buf, &len) == 0;
#elif defined(__linux__)
    ssize_t len = readlink("/proc/self/exe", buf, size - 1);
    if (len <= 0)
        return 0;
    buf[len] = '\0';
    return 1;
#else
    (void)buf;
    (void)size;
    return 0;
#endif
}

int myna_stream_is_terminal(FILE *stream)
{
#ifdef _WIN32
    return _isatty(_fileno(stream)) != 0;
#else
    return isatty(fileno(stream));
#endif
}

#ifdef _WIN32
int myna_copy_to_clipboard(const char *text)
{
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (wlen <= 0)
        return 0;
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (size_t)wlen * sizeof(wchar_t));
    if (!mem)
        return 0;
    wchar_t *wide = GlobalLock(mem);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, wlen);
    GlobalUnlock(mem);

    if (!OpenClipboard(NULL)) {
        GlobalFree(mem);
        return 0;
    }
    EmptyClipboard();
    int ok = SetClipboardData(CF_UNICODETEXT, mem) != NULL;
    CloseClipboard();
    if (!ok)
        GlobalFree(mem); /* on success the clipboard owns it */
    return ok;
}
#else
/* Pipe `text` into `command` if `tool` is installed. */
static int pipe_to(const char *tool, const char *command, const char *text)
{
    char check[128];
    snprintf(check, sizeof check, "command -v %s >/dev/null 2>&1", tool);
    if (system(check) != 0)
        return 0;

    FILE *p = popen(command, "w");
    if (!p)
        return 0;
    /* A tool that exits early must not kill Myna with SIGPIPE. */
    void (*old)(int) = signal(SIGPIPE, SIG_IGN);
    int wrote = fputs(text, p) >= 0;
    int status = pclose(p);
    signal(SIGPIPE, old);
    return wrote && status == 0;
}

int myna_copy_to_clipboard(const char *text)
{
#ifdef __APPLE__
    return pipe_to("pbcopy", "pbcopy", text);
#else
    const char *wayland = getenv("WAYLAND_DISPLAY");
    const char *x11 = getenv("DISPLAY");
    if (wayland && *wayland && pipe_to("wl-copy", "wl-copy 2>/dev/null", text))
        return 1;
    if (x11 && *x11) {
        if (pipe_to("xclip", "xclip -selection clipboard 2>/dev/null", text))
            return 1;
        if (pipe_to("xsel", "xsel --clipboard --input 2>/dev/null", text))
            return 1;
    }
    return 0;
#endif
}
#endif
