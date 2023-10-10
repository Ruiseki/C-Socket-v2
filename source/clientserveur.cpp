#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <WinSock2.h>

#include "reseau.hpp"
#include "client.hpp"
#include "serveur.hpp"

int lancerClient(Client* client)
{
	return 0;
}

int lancerServeur(Serveur* serveur)
{
	return 0;
}

int main()
{
	Client client;
	Serveur serveur;
	int selection, returnCode = 0;
	
	do
	{
		std::cout << "1: Serveur" << std::endl << "2: Client" << std::endl;
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
	} while (selection < 1 || selection > 2);

	return returnCode;
}