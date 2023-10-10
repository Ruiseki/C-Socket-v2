#include "serveur.hpp"
#include <WinSock2.h>
#include <string>
#include <thread>
#include <fstream>
#include <mutex>

Serveur::Serveur()
{
}

Serveur::~Serveur()
{
}

std::thread Serveur::listenerSpawnThread(int port, std::string protocole)
{
    return std::thread();
}

void Serveur::listener(int port, std::string protocole)
{
}