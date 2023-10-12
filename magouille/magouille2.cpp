#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <windows.h>
#include <algorithm>
using namespace std;

int getConsoleHeight(HANDLE hConsole)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top;
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
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout << "Chat :";
    
    int begin = messages->size() < getConsoleHeight(hConsole) - 6 ? 0 : messages->size() - getConsoleHeight(hConsole) + 6;

    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    for(int i = 0; i + begin < messages->size(); i++)
    {
        moveCursor(hConsole, 0, 3 + i);
        cout << "\033[2K\r";
        moveCursor(hConsole, 4, 3 + i);
        cout << messages->at(begin + i);
    }

    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    moveCursor(hConsole, 0, getConsoleHeight(hConsole) - 1);
    cout << "Type your message : ";
}

void changeHeight(HANDLE hConsole, vector<string>* messages, bool* stop)
{
    int currentHeight = getConsoleHeight(hConsole);
    while(!*stop)
    {
        if( currentHeight != getConsoleHeight(hConsole))
        {
            system("cls");
            currentHeight = getConsoleHeight(hConsole);
            updateFrame(hConsole, messages);
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
        }
        this_thread::sleep_for(chrono::milliseconds(1)); // drain to much ressources otherwise
    }
}

int main(int argc, char const *argv[])
{
    system("cls");
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    vector<string> messages;
    string myMessage;
    bool stop = false;

    thread taskChangeHeight(changeHeight, hConsole, &messages, &stop);

    do
    {
        updateFrame(hConsole, &messages);

        SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
        getline(cin, myMessage);
        moveCursor(hConsole, 0, getConsoleHeight(hConsole) - 1);
        cout << "\033[2K\r";

        messages.push_back(myMessage);

    } while(myMessage != ">quit");

    stop = true;
    taskChangeHeight.join();
    system("cls");
    return 0;
}