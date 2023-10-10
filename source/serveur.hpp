#ifndef SERVEUR_HPP
#define SERVEUR_HPP

#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <mutex>
#include <WinSock2.h>

#include "reseau.hpp"

class Serveur : public Reseau
{
public:
	Serveur();
	~Serveur();

	std::thread listenerSpawnThread(int port, std::string protocole);

private:
	void listener(int port, std::string protocole);

};

#endif