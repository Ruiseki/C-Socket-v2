#ifndef CHAT_HPP
#define CHAT_HPP

#include <vector>
#include <string>
#include <windows.h>

#include "reseau.hpp"

int getConsoleHeight();

COORD getConsoleCursorPosition();

void moveCursor(int x, int y);

void updateFrame();

void gestionTailleFenetre(bool* stop);

void* lireServeur(std::string message);

void* recepetionMessageClient(std::string message);

int lancerClient();

int lancerServeur();

int mainMenu();

#endif