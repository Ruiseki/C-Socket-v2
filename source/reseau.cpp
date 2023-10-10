#include <WinSock2.h>
#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <fstream>

#include "reseau.hpp"

Reseau::Reseau(int maxConnection, int tailleBuffer, int tailleBufferDonnee) : maxConnexion(maxConnection), bufferSize(tailleBuffer), dataBufferSize(tailleBufferDonnee)
{
    connexions = new connexion[maxConnexion];
}

Reseau::Reseau(int maxConnection) : Reseau(maxConnection, 128, 1024)
{
    
}

Reseau::Reseau() : Reseau(4, 128, 1024)
{ }

Reseau::~Reseau()
{
    delete[] connexions;
}

bool Reseau::listenerInit(int port, std::string protocole)
{
    if( !this->initWsa() ) return false;
    // Déclaration du socket d'écoute
    if( !this->initSocket(protocole, _sockfdEcoute) ) return false;
    // Déclaration addr écoute
    if( !this->initAdresseSocket(adresseSocketEcoute, "aucune", port) ) return false;

    // binding
    int errorCode = bind(_sockfdEcoute, (sockaddr*)&adresseSocketEcoute, sizeof(adresseSocketEcoute));
    if( errorCode == SOCKET_ERROR )
    {
        std::cout << "BINDING ERROR : " << std::endl << WSAGetLastError() << std::endl;
        return false;
    }

    // le socket écoute un port de la machine
    if( listen(_sockfdEcoute, maxConnexion) < 0)
    {
        std::cout << "LISTEN ERROR" << std::endl;
        return false;
    }
    return true;
}

bool Reseau::initWsa()
{
    this->wlog("---- WSA");
    if( WSAStartup(MAKEWORD(2, 2), &wsaData) < 0 )
    {
        this->wlog("--- ERROR : " + WSAGetLastError());
        return false;
    }
    this->wlog("CHECK");
    return true;
}

bool Reseau::initAdresseSocket(sockaddr_in &addr, std::string adresseCible, int port)
{
    this->wlog("---- ADDR");
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;   // AF_INET est utilisé pour les protocole TCP et UDP

    // CLIENT
    if(adresseCible != "aucune")
    {
        addr.sin_addr.s_addr = inet_addr( adresseCible.c_str() ); // Indique l'adresse du serveur
        if(addr.sin_addr.s_addr <= 0)
        {
            this->wlog("ERROR : " + WSAGetLastError());
            return false;
        }
    }
    // SERVEUR
    else
    {
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    memset( &(addr.sin_zero), 0, 8 );

    this->wlog("CHECK");
    return true;
}

bool Reseau::initSocket(std::string protocole, int &sockfd)
{
    this->wlog("---- SOCKET");
    sockfd= protocole == "tcp" ? socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) :
            protocole == "udp" ? socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) :
            -1;

    if(sockfd < 0)
    {
        this->wlog("ERROR : " + WSAGetLastError());
        return false;
    }
    this->wlog("CHECK");
    return true;
}

void Reseau::envoyerText(int idConnection, std::string text)
{
    if(idConnection < 0 || idConnection > maxConnexion) return;
    else if(!connexions[idConnection].estLibre)
    {
        send(connexions[idConnection].sockfd, text.c_str(), text.size(), 0);
    }
}

void Reseau::envoyerText(std::string text)
{
    // ...
}

void Reseau::setNomConnection(int idConnection, std::string nom)
{

}

void Reseau::wlog(std::string logMessage)
{
    std::ofstream wlog("./log.txt", std::ios::app);
    wlog << logMessage << std::endl;
}