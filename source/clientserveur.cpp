#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <WinSock2.h>
#include <windows.h>

#include "reseau.hpp"
#include "chat.hpp"

/*
	TO DO

	- Le programme plante en mode client quand on quitte
	- Le programme en mode client se comporte étrangement quand un message arrive
			et qu'on est en train d'écrire

*/

void lireServeur(Reseau* serveur)
{
	while(!serveur->stopListener)
	{
		if(serveur->dataQueue.size() > 0)
		{
			for(std::string message : serveur->dataQueue)
			{
				std::cout << message << std::endl;
				for(int id : serveur->connexionsActives)
				{
					serveur->envoyer(id, message);
				}
			}
			serveur->locker.lock();
			serveur->dataQueue.clear();
			serveur->locker.unlock();

		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

int lancerClient(Reseau* client, HANDLE hConsole)
{
	// configuration
	std::string pseudoDef = "Unknow", adressedef = "92.95.32.114", portdef = "55555", portEcoutedef = "55556";
	std::string pseudo, adresse, portstr, portEcoutestr;
	int port, portEcoute;
	system("cls");

	std::cin.ignore();
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::cout << "Pseudo : ";
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
	std::getline(std::cin, pseudo);
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::cout << "Adresse (" << adressedef << ") : ";
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
	std::getline(std::cin, adresse);
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::cout << "Port destination (" << portdef << ") : ";
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
	std::getline(std::cin, portstr);
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::cout << "Port d'ecoute (" << portEcoutedef << ") : ";
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
	std::getline(std::cin, portEcoutestr);

	pseudo = pseudo == "" ? pseudoDef : pseudo;
	adresse = adresse ==  "" ? adressedef : adresse;
	port = portstr == "" ? std::stoi(portdef) : std::stoi(portstr);
	portEcoute = portEcoutestr == "" ? std::stoi(portEcoutedef) : std::stoi(portEcoutestr);

	system("cls");

	// connexion
	std::cout << "Connexion ..." << std::endl;
	int id = client->connection(adresse, port, "tcp");
	if(id < 0)
	{
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
		std::cout << "Connexion impossible\nErreur " << id << std::endl << std::endl;
		return -1;
	}
	system("cls");
	std::cout << "Connecter" << std::endl << std::endl;
	system("cls");

	// chat
	std::string message;

	std::thread tacheObservateur = client->observateur(portEcoute, "tcp");
	std::thread tacheArrierePlan(backgroundWork, hConsole, &client->dataQueue, &client->stopListener);

	do
	{
		updateFrame(hConsole, &client->dataQueue);

        std::getline(std::cin, message);
        moveCursor(hConsole, 0, getConsoleHeight(hConsole) - 1);
        std::cout << "\033[2K\r";

		if(message != ">quit")
		{
			message = pseudo + " : " + message;
			client->envoyer(id, message);
		}

	} while( message != ">quit" );

	system("cls");

	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::cout << "Arret des threads ..." << std::endl;
	client->stopListener = true;
	tacheArrierePlan.join();
	tacheObservateur.join();
	std::cout << "Threads terminer" << std::endl;

	std::cout << "Arret de la conversation...";
	if(!client->terminerConnection(id))
	{
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
		std::cout << "Erreur lors de la déconnexion";
	}

	system("cls");
	return 0;
}

int lancerServeur(Reseau* serveur, HANDLE hConsole)
{
	system("cls");
	std::thread tacheObservateur = serveur->observateur(55555, "tcp");
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::cout << "Serveur demarrer sur le port 55555" << std::endl;
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
	std::cout << "Appuyez sur 'q' pour quitter le mode serveur" << std::endl << std::endl;
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::thread tacheTraitementMessage(lireServeur, serveur);

	while(!((GetKeyState('Q') & 0x8000) && GetConsoleWindow() == GetForegroundWindow()))
	{ }

	system("cls");
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::cout << "Arret du serveur ...";
	serveur->stopListener = true;
	tacheTraitementMessage.join();
	tacheObservateur.join();
	system("cls");
	std::cin.ignore();
	return 0;
}

int main()
{
	Reseau client;
	Reseau serveur;
	int selection, returnCode = 0;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	system("cls");

	do
	{
		SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
		std::cout << "1: Lancer le client" << std::endl << "2: Lancer le serveur" << std::endl;
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
		std::cout << "3: Quitter" << std::endl;
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

		std::cin >> selection;
		switch (selection)
		{
		case 1:
			returnCode = lancerClient(&client, hConsole);
			break;
		case 2:
			returnCode = lancerServeur(&serveur, hConsole);
			break;

		default:
		system("cls");
			break;
		}
	} while (selection != 3);

	system("cls");
	return returnCode;
}