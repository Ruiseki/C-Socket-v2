#ifndef CHAT_HPP
#define CHAT_HPP

#include <vector>
#include <string>
#include <windows.h>

int getConsoleHeight(HANDLE hConsole);

void moveCursor(HANDLE hConsole, int x, int y);

void updateFrame(HANDLE hConsole, std::vector<std::string>* messages);

void backgroundWork(HANDLE hConsole, std::vector<std::string>* messages, bool* stop);

#endif