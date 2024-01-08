#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>

#include "chat.hpp"
#include "reseau.hpp"

using namespace std;

Reseau serveur;
Reseau client;
vector<string> messages;
HANDLE hConsole;

int getConsoleHeight()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top;
}

COORD getConsoleCursorPosition()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    return csbi.dwCursorPosition;
}

void moveCursor(int x, int y)
{
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(hConsole, coord);
}

void updateFrame()
{
    moveCursor(0, 0);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
    cout << "Type \">quit\" to exit";
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    moveCursor(0, 2);
    cout << "\tChat :";

    int begin = messages.size() < getConsoleHeight() - 6 ? 0 : messages.size() - getConsoleHeight() + 6;

    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    for (int i = 0; i + begin < messages.size(); i++)
    {
        moveCursor(0, 3 + i);
        cout << "\033[2K\r";
        moveCursor(4, 3 + i);
        cout << "\t\t" << messages.at(begin + i) << endl;
    }

    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    moveCursor(0, getConsoleHeight() - 1);
    cout << "Type your message : ";
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
}

void gestionTailleFenetre(bool* stop)
{
    int currentHeight = getConsoleHeight();
    int oldSize = 0;

    while (!*stop)
    {
        if (currentHeight != getConsoleHeight())
        {
            COORD currentCursorPos = getConsoleCursorPosition();
            system("cls");
            updateFrame();
            if (currentHeight < getConsoleHeight())
                moveCursor(currentCursorPos.X, currentCursorPos.Y + 1);
            else
                moveCursor(currentCursorPos.X, currentCursorPos.Y - 1);
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
            currentHeight = getConsoleHeight();
        }

        if (messages.size() > oldSize)
        {
            oldSize = messages.size();
            COORD currentCursorPos = getConsoleCursorPosition();
            updateFrame();
            moveCursor(currentCursorPos.X, currentCursorPos.Y);
        }
        this_thread::sleep_for(chrono::milliseconds(1)); // drain to much ressources otherwise
    }
}

void* lireServeur(string message)
{
	cout << message << endl;
	for (int id : serveur.connexionsActives)
	{
		serveur.envoyer(id, message);
	}
	return nullptr;
}

void* recepetionMessageClient(string message)
{
	messages.push_back(message);
	updateFrame();
	return nullptr;
}

int lancerClient()
{
	// configuration
	string pseudoDef = "Unknown", adressedef = "127.0.0.1";
	int portdef = 55555, portEcoutedef = 55556;
	string pseudo, adresse, portstr, portEcoutestr;
	int port, portEcoute;
	system("cls");

	cin.ignore();
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	cout << "Pseudo : ";
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
	getline(cin, pseudo);
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	cout << "Adresse (" << adressedef << ") : ";
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
	getline(cin, adresse);
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	cout << "Port destination (" << portdef << ") : ";
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
	getline(cin, portstr);
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	cout << "Port d'ecoute (" << portEcoutedef << ") : ";
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
	getline(cin, portEcoutestr);

	pseudo = pseudo == "" ? pseudoDef : pseudo;
	adresse = adresse == "" ? adressedef : adresse;
	port = portstr == "" ? portdef : stoi(portstr);
	portEcoute = portEcoutestr == "" ? portEcoutedef : stoi(portEcoutestr);

	system("cls");

	// connexion
	cout << "Connexion ..." << endl;
	int id = client.connection(adresse, port, "tcp");
	if (id < 0)
	{
		system("cls");
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
		cout << "Connexion impossible\nErreur " << id << endl << endl;
		return -1;
	}
	system("cls");
	cout << "Connecter" << endl << endl;
	system("cls");

	// chat
	string message;

	client.stopObservateur = false;
	thread tacheObservateur = client.observateurThread(portEcoute, "tcp", false, &recepetionMessageClient);
	thread tacheArrierePlan(gestionTailleFenetre, &client.stopObservateur);

	do
	{
		updateFrame();

		getline(cin, message);
		moveCursor(0, getConsoleHeight() - 1);
		cout << "\033[2K\r";

		if (message != ">quit")
		{
			message = pseudo + " : " + message;
			client.envoyer(id, message);
		}

	} while (message != ">quit");

	system("cls");

	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	cout << "Arret des threads ..." << endl;
	client.stopObservateur = true;
	tacheArrierePlan.join();
	tacheObservateur.join();
	cout << "Threads terminer" << endl;

	cout << "Arret de la conversation...";
	if (!client.terminerConnection(id))
	{
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
		cout << "Erreur lors de la déconnexion";
	}

	system("cls");
	return 0;
}

int lancerServeur()
{
	// configuration
	int port;
	int portdef = 55555;
	string portstr;
	system("cls");

	cin.ignore();
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	cout << "Port du serveur : (" << portdef << ") : ";
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	getline(cin, portstr);

	port = portstr == "" ? portdef : stoi(portstr);

	serveur.stopObservateur = false;
	thread tacheObservateur = serveur.observateurThread(port, "tcp", true, &lireServeur);

	system("cls");
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	cout << "Serveur demarre sur le port " << port << endl;
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
	cout << "Appuyez sur 'q' pour quitter le mode serveur" << endl << endl;
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

	HWND fenetreProgramme = GetForegroundWindow();

	while (!((GetKeyState('Q') & 0x8000) && fenetreProgramme == GetForegroundWindow()))
	{
		this_thread::sleep_for(chrono::milliseconds(1));
	}

	system("cls");
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	cout << "Arret du serveur ...";
	serveur.stopObservateur = true;
	tacheObservateur.join();
	system("cls");
	return 0;
}

int mainMenu()
{
	int selection = 3, returnCode = 0;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	system("cls");

	do
	{
		SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
		cout << "1: Lancer le client" << endl << "2: Lancer le serveur" << endl;
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
		cout << "3: Quitter" << endl;
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		cin >> selection;

		// besoin de check si l'input est autre chose qu'un nombre
		switch (selection)
		{
		case 1:
			returnCode = lancerClient();
			break;
		case 2:
			returnCode = lancerServeur();
			break;

		default:
			system("cls");
			break;
		}
	} while (selection != 3);

	system("cls");
	return returnCode;
}