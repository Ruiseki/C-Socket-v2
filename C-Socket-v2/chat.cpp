#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <windows.h>
#include <algorithm>

#include "chat.hpp"

using namespace std;

int getConsoleHeight(HANDLE hConsole)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top;
}

COORD getConsoleCursorPosition(HANDLE hConsole)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    return csbi.dwCursorPosition;
}

void moveCursor(HANDLE hConsole, int x, int y)
{
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(hConsole, coord);
}

void updateFrame(HANDLE hConsole, vector<string>* messages)
{
    moveCursor(hConsole, 0, 0);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
    cout << "Type \">quit\" to exit";
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    moveCursor(hConsole, 0, 2);
    cout << "\tChat :";

    int begin = messages->size() < getConsoleHeight(hConsole) - 6 ? 0 : messages->size() - getConsoleHeight(hConsole) + 6;

    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    for (int i = 0; i + begin < messages->size(); i++)
    {
        moveCursor(hConsole, 0, 3 + i);
        cout << "\033[2K\r";
        moveCursor(hConsole, 4, 3 + i);
        cout << messages->at(begin + i) << endl;
    }

    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    moveCursor(hConsole, 0, getConsoleHeight(hConsole) - 1);
    cout << "Type your message : ";
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
}

void backgroundWork(HANDLE hConsole, vector<string>* messages, bool* stop)
{
    int currentHeight = getConsoleHeight(hConsole);
    int oldSize = 0;

    while (!*stop)
    {
        if (currentHeight != getConsoleHeight(hConsole))
        {
            COORD currentCursorPos = getConsoleCursorPosition(hConsole);
            system("cls");
            updateFrame(hConsole, messages);
            if (currentHeight < getConsoleHeight(hConsole))
                moveCursor(hConsole, currentCursorPos.X, currentCursorPos.Y + 1);
            else
                moveCursor(hConsole, currentCursorPos.X, currentCursorPos.Y - 1);
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
            currentHeight = getConsoleHeight(hConsole);
        }

        if (messages->size() > oldSize)
        {
            oldSize = messages->size();
            COORD currentCursorPos = getConsoleCursorPosition(hConsole);
            updateFrame(hConsole, messages);
            moveCursor(hConsole, currentCursorPos.X, currentCursorPos.Y);
        }
        this_thread::sleep_for(chrono::milliseconds(1)); // drain to much ressources otherwise
    }
}