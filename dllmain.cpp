// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#define WIN32_LEAN_AND_MEAN

#include <thread>
#include <string>
#include <iostream>
#include <vector>
#include <conio.h>
#include <sstream>
#include <Windows.h>
#include <shellapi.h>
#include <detours.h>
#include <iostream>
#include <filesystem>
#include "utils.h"
#include "AutocompleteUI.h"

#pragma comment( lib, "detours.lib" )

static __int64 (__cdecl* RealDoComplete)(unsigned __int16*, int, int, int, int, int);
static BOOL(WINAPI* RealReadConsoleW)(HANDLE  hConsoleInput, LPVOID  lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, PCONSOLE_READCONSOLE_CONTROL pInputControl) = ReadConsoleW;
static int (WINAPI* RealEntryPoint)(VOID) = NULL;

static BOOL WINAPI HookedReadConsoleW(HANDLE  hConsoleInput, LPVOID  lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, PCONSOLE_READCONSOLE_CONTROL pInputControl)
{
    return RealReadConsoleW(hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, pInputControl);
}

static std::vector<std::wstring> ParseCmdline(std::wstring cmdline)
{
    std::vector<std::wstring> args;
    int argCount;

    if (cmdline.length() == 0)
        return args;

    LPWSTR* res = CommandLineToArgvW((LPCWSTR)cmdline.c_str(), &argCount);
    for (int i = 0; i < argCount; i++)
    {
        args.push_back(std::wstring(res[i]));
    }

    // If there is space after the last arg, add another empty arg.
    WCHAR c = cmdline[cmdline.length() - 1];
    if (args.size() > 0 && c == ' ')
    {
        args.push_back(L"");
    }

    return args;
}

static std::vector<std::wstring> GetFilesAndDirsWithPath(std::wstring path)
{
    std::vector<std::wstring> list;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW ffd;

    hFind = FindFirstFileW(path.c_str(), &ffd);
    int err = GetLastError();
    if (INVALID_HANDLE_VALUE == hFind)
    {
        return list;
    }

    do
    {
        list.push_back(ffd.cFileName);
    } while (FindNextFileW(hFind, &ffd) != 0);

    FindClose(hFind);

    return list;
}

static std::vector<std::wstring> GetAutoCompleteOptions(std::vector<std::wstring> argv, std::wstring& argToComplete)
{
    std::vector<std::wstring> options;
    argToComplete = L"";

    if (argv.size() == 0)
    {
        // HANDLE COMMAND OPTIONS ETC
        std::wstring glob = L"*";
        auto o = GetFilesAndDirsWithPath(glob);
        for (int i = 0; i < o.size(); i++)
        {
            options.push_back(o[i]);
        }
    }
    else
    {
        argToComplete = argv[argv.size() - 1];
        std::wstring glob = argToComplete + L"*";

        int rFind = glob.find_last_of(L'\\');
        auto baseO = glob.substr(0, rFind+1);

        auto o = GetFilesAndDirsWithPath(glob);
        for (int i = 0; i < o.size(); i++)
        {
            auto s = baseO + o[i];
            options.push_back(s);
        }
    }

    return options;
}

static __int64 __cdecl HookedDoComplete(unsigned __int16* startText, int b, int startTextLen, int d, int e, int isFirst)
{
    std::wstring startStr = std::wstring((WCHAR*)startText, startTextLen);

    auto argv = ParseCmdline(startStr);

    std::wstring argToComplete;
    auto options = GetAutoCompleteOptions(argv, argToComplete);
    return AutoComplete(options, startStr, argToComplete);
}

static int WINAPI ExtendEntryPoint()
{
    /*for (HMODULE hModule = NULL; (hModule = DetourEnumerateModules(hModule)) != NULL;) {
        CHAR szName[MAX_PATH] = { 0 };
        GetModuleFileNameA(hModule, szName, sizeof(szName) - 1);
        printf("  %p: %s\n", hModule, szName);
    }*/

    HMODULE cmdModuleHandle = GetModuleHandleA(NULL);
    RealDoComplete = (__int64 (*__cdecl)(unsigned __int16*, int, int, int, int, int))((long long)cmdModuleHandle + 0x2eadc);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)RealReadConsoleW, HookedReadConsoleW);
    DetourAttach(&(PVOID&)RealDoComplete, HookedDoComplete);
    DetourTransactionCommit();

    hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);

    return RealEntryPoint();
}

BOOL __declspec(dllexport) DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;    

    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    if (dwReason == DLL_PROCESS_ATTACH) {
        DetourRestoreAfterWith();

        RealEntryPoint = (int (WINAPI*)())DetourGetEntryPoint(NULL);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)RealEntryPoint, ExtendEntryPoint);
        error = DetourTransactionCommit();
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        error = DetourTransactionCommit();
    }
    return TRUE;
}