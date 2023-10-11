#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <WinSock2.h>
#include <windows.h>

#include "reseau.hpp"

void lireMessage(Reseau* network)
{
	while(!network->stopListener)
	{
		if(network->dataQueue.size() > 0)
		{
			for(std::string message : network->dataQueue)
			{
				std::cout << message << std::endl;
			}
			network->locker.lock();
			network->dataQueue.clear();
			network->locker.unlock();
		}
	}
}

int lancerClient(Reseau* client)
{
	// configuration
	std::string adressedef = "92.95.32.114", portdef = "55555", portEcoutedef = "55556";
	std::string adresse, portstr, portEcoutestr;
	int port, portEcoute;
	system("cls");

	std::cin.ignore();
	std::cout << "Adresse (" << adressedef << ") : ";
	std::getline(std::cin, adresse);
	std::cout << "Port destination (" << portdef << ") : ";
	std::getline(std::cin, portstr);
	std::cout << "Port d'ecoute (" << portEcoutedef << ") : ";
	std::getline(std::cin, portEcoutestr);

	adresse = adresse ==  "" ? adressedef : adresse;
	port = portstr == "" ? std::stoi(portdef) : std::stoi(portstr);
	portEcoute = portEcoutestr == "" ? std::stoi(portEcoutedef) : std::stoi(portEcoutestr);

	system("cls");

	// connexion
	std::cout << "Connexion ..." << std::endl;
	int id = client->connection(adresse, port, "tcp");
	if(id < 0)
	{
		std::cout << "Connexion impossible\nErreur " << id << std::endl << std::endl;
		return -1;
	}
	system("cls");
	std::cout << "Connecter" << std::endl << std::endl;

	// chat
	std::thread tacheObservateur = client->listenerSpawnThread(portEcoute, "tcp");
	std::thread tacheLireMessage(lireMessage, client);

	std::string message;
	do
	{
		std::getline(std::cin, message);
		if(message != ">quit")
			client->envoyerText(id, message);
	} while( message != ">quit" );

	client->terminerConnection(id);
	client->stopListener = true;

	tacheLireMessage.join();
	tacheObservateur.join();

	system("cls");
	return 0;
}

int lancerServeur(Reseau* serveur)
{
	system("cls");
	std::thread tacheObservateur = serveur->listenerSpawnThread(55555, "tcp");
	std::cout << "Serveur demarrer sur le port 55555" << std::endl << "Appuyez sur 'q' pour quitter le mode serveur" << std::endl;
	std::thread tacheLireMessage(lireMessage, serveur);

	while(!((GetKeyState('Q') & 0x8000) && GetConsoleWindow() == GetForegroundWindow()))
	{ }

	serveur->stopListener = true;
	tacheLireMessage.join();
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
	system("cls");
	
	do
	{
		std::cout << "1: Lancer le client" << std::endl << "2: Lancer le serveur" << std::endl << "3: Quitter" << std::endl;
		std::cin >> selection;
		switch (selection)
		{
		case 1:
			returnCode = lancerClient(&client);
			break;
		case 2:
			returnCode = lancerServeur(&serveur);
			break;

		default:
		system("cls");
			break;
		}
	} while (selection != 3);

	system("cls");
	return returnCode;
}