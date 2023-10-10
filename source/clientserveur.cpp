#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <WinSock2.h>

#include "reseau.hpp"

void lireMessage(Reseau* network)
{
	while(!network->stopListener)
	{
		if(network->dataQueue.size() > 0)
		{
			for(Reseau::messageInfo message : network->dataQueue)
			{
				std::cout << message.nom << ": " << message.message << std::endl;
			}
			network->locker.lock();
			network->dataQueue.clear();
			network->locker.unlock();
		}
	}
}

int lancerClient(Reseau* client)
{
	std::string pseudo;
	system("cls");
	std::cout << "Username : ";
	std::cin >> pseudo;
	system("cls");

	std::cout << "Connexion ..." << std::endl;
	int id = client->connection("127.0.0.1", 55555, "tcp");
	if(id < 0)
	{
		std::cout << "Connexion impossible\nErreur " << id << std::endl;
		return -1;
	}
	system("cls");
	std::cout << "Connecter" << std::endl << std::endl;

	std::thread tacheObservateur = client->listenerSpawnThread(55556, "tcp");
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

	return 0;
}

int lancerServeur(Reseau* serveur)
{
	system("cls");
	std::thread tacheObservateur = serveur->listenerSpawnThread(55555, "tcp");
	std::cout << "Server started on port 55555" << std::endl;

	std::thread tacheLireMessage(lireMessage, serveur);

	tacheLireMessage.join();
	tacheObservateur.join();
	return 0;
}

int main()
{
	Reseau client;
	Reseau serveur;
	int selection, returnCode = 0;
	
	do
	{
		system("cls");
		std::cout << "1: Client" << std::endl << "2: Serveur" << std::endl;
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
			break;
		}
	} while (selection < 1 || selection > 2);

	return returnCode;
}