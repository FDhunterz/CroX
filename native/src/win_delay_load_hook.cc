/*
 * Delay-load hook for Electron on Windows (node.exe imports).
 * Based on node-gyp / cmake-js; avoids GetProcAddress(NULL) before the host module is set.
 */

#ifdef _MSC_VER

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <delayimp.h>
#include <string.h>

static HMODULE host_module = NULL;

static HMODULE resolve_host_module() {
  if (host_module) return host_module;
  host_module = GetModuleHandleA("node.dll");
  if (!host_module) host_module = GetModuleHandleA("libnode.dll");
  if (!host_module) host_module = GetModuleHandleA(NULL);
  return host_module;
}

static FARPROC WINAPI load_exe_hook(unsigned int event, DelayLoadInfo* info) {
  if (event == dliNotePreGetProcAddress) {
    HMODULE host = resolve_host_module();
    if (!host) return NULL;
    FARPROC proc = GetProcAddress(host, info->dlp.szProcName);
    if (proc) return proc;
    return NULL;
  }
  if (event != dliNotePreLoadLibrary) return NULL;
  if (_stricmp(info->szDll, "node.exe") != 0) return NULL;
  return reinterpret_cast<FARPROC>(resolve_host_module());
}

decltype(__pfnDliNotifyHook2) __pfnDliNotifyHook2 = load_exe_hook;

#endif
