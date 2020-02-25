#pragma once
#include <string>
#include <iostream>
#include <Windows.h>
#include <codecvt>

static COORD GetCursorPos()
{
    CONSOLE_SCREEN_BUFFER_INFO cInfo;
    auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleScreenBufferInfo(hConsole, &cInfo);
    return cInfo.dwCursorPosition;
}

void WriteToFile(HANDLE hFile, std::string format)
{
    LPCSTR output = new CHAR[1024] + 1;
    WriteFile(hFile, format.c_str(), format.length(), NULL, NULL);
}

void WriteToFile(HANDLE hFile, std::string format, int var)
{
    LPCSTR output = new CHAR[1024] + 1;
    int written = std::snprintf((char*)output, 1024, format.c_str(), var);
    WriteFile(hFile, output, written, NULL, NULL);
}

static void InjectString(HANDLE hConsoleInput, std::wstring str)
{
    INPUT_RECORD* inputs = new INPUT_RECORD[str.length()];
    for (int i = 0; i < str.length(); i++)
    {
        INPUT_RECORD in;
        in.EventType = KEY_EVENT;
        in.Event.KeyEvent.bKeyDown = TRUE;
        in.Event.KeyEvent.wRepeatCount = 1;
        in.Event.KeyEvent.uChar.UnicodeChar = str[i];

        inputs[i] = in;
    }

    int out;
    WriteConsoleInputW(hConsoleInput, inputs, str.length(), (LPDWORD)&out);
}

static std::string ws2s(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}