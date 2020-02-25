#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <Windows.h>
#include "utils.h"

// General
static HANDLE hConsole = NULL;
static HANDLE hConsoleInput = NULL;
static CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

static int MAX_ROWS = 5;

// State keeping
static bool boxOpen = false;
static int boxLength = -1;
static std::wstring originalText;
static COORD originalCurPos;
static int boxRows = 0;
static int boxColumns = 0;

static void DisplayAutoCompleteBox(std::vector<std::wstring> options, int selectedIndex = -1)
{
    auto curPos = GetCursorPos();
    
    int columns = 1;
    int rows = MAX_ROWS;
    while (options.size() > rows * columns)
    {
        int option_max_length = -1;
        for (int i = 0; i < options.size(); i++)
        {
            int l = options[i].length();
            option_max_length = max(l, option_max_length);
        }

        if (option_max_length < consoleInfo.dwSize.X / (columns + 1))
            columns += 1;
        else
            break;
    }

    boxRows = rows;
    boxColumns = columns;

    wprintf(L"\r\n");
    for (int i = 0; i < rows; i++)
    {
        int column_width = consoleInfo.dwSize.X / columns;
        for (int j = 0; j < columns; j++)
        {
            int index = j * rows + i;
            if (index >= options.size())
                continue;

            if (selectedIndex == index) {
                SetConsoleTextAttribute(hConsole, 0x80);
            }
            else {
                SetConsoleTextAttribute(hConsole, 0x70);
            }
            wprintf(L"%-*s", column_width, options[index].c_str());
            SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
        }
    }

    SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
    SetConsoleCursorPosition(hConsole, curPos);
}

static void HideAutoCompleteBox()
{
    if (!boxOpen)
        return;

    auto originalCurPos = GetCursorPos();
    boxOpen = false;
    for (int i = 0; i < boxRows; i++)
    {
        printf("\n%*s", consoleInfo.dwSize.X, "");
    }
    SetConsoleCursorPosition(hConsole, originalCurPos);
}

static int AutoComplete(std::vector<std::wstring> options, std::wstring startText, std::wstring argToComplete)
{
    if (options.empty())
        return 0;
    auto promptCurPos = GetCursorPos();
    promptCurPos.X -= startText.length();

    boxOpen = true;
    boxLength = options.size();
    originalCurPos = GetCursorPos();
    originalText = startText;

    DisplayAutoCompleteBox(options);

    std::wstring toBeWritten = L"";
    int selectedIndex = -1;
    while (true)
    {
        bool special = false;
        wchar_t c = _getwch();
        if (c == 0xE0) {
            special = true;
            c = _getwch();
        }

        if (c == VK_ESCAPE) {
            break;
        }
        // Down
        else if (c == VK_TAB || (special && c == 0x50)) {
            selectedIndex += 1;
        }
        // Up
        else if (special && c == 0x48) {
            selectedIndex += -1;
        }
        // Right
        else if (special && c == 0x4D) {
            selectedIndex += boxRows;
        }
        // Left
        else if (special && c == 0x4B) {
            selectedIndex += -boxRows;
        }
        else if (c == VK_RETURN) {
            if (selectedIndex != -1)
            {
                for (int i = 0; i < argToComplete.length(); i++)
                    toBeWritten.push_back(L'\b');
                toBeWritten += options[selectedIndex];
            }
            break;
        }
        else {
            toBeWritten = std::wstring(1, c);
            break;
        }
        
        // Clamp
        selectedIndex = min(options.size() - 1, max(0, selectedIndex));
        
        auto parameterCurPos = originalCurPos;
        parameterCurPos.X -= argToComplete.length();

        DisplayAutoCompleteBox(options, selectedIndex);

        // Clean the argument console space
        SetConsoleCursorPosition(hConsole, parameterCurPos);
        printf("%-*s", consoleInfo.dwSize.X - parameterCurPos.X, "");
        SetConsoleCursorPosition(hConsole, parameterCurPos);

        // Print the current option
        wprintf(L"%s", options[selectedIndex].c_str());
    }

    HideAutoCompleteBox();
    if (toBeWritten.empty() == false)
        InjectString(hConsoleInput, toBeWritten);

    return 1;
}



























