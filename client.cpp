#include "client.hpp"
#include <WinSock2.h>
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <thread>
#include <iostream>

Client::Client(int maxConnection, int tailleBuffer, int tailleBufferDonnee) : _maxConnection(maxConnection), bufferSize(tailleBuffer), dataBufferSize(tailleBufferDonnee)
{
    if( !this->initWsa() ) this->~Client();
    _connections = new _connection[_maxConnection];
}

Client::Client(int maxConnection) : Client(maxConnection, 128, 1024)
{ }

Client::Client() : Client(4, 128, 1024)
{ }

Client::~Client()
{
    delete[] _connections;
    _connections = 0;
}

bool Client::listenerInit(int port, std::string protocole)
{
    std::cout << "Declation du socket d'ecoute." << std::endl;
    // Déclaration du socket d'écoute
    _sockfdEcoute = protocole == "tcp" ? socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) :
                       protocole == "udp" ? socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) :
                       -1;

    if(_sockfdEcoute < 0) return false;

    std::cout << "Declaration addr." << std::endl;
    // Déclaration addr écoute
    _adresseInternetEcoute.sin_addr.s_addr = INADDR_ANY;
    _adresseInternetEcoute.sin_port = htons(port);
    _adresseInternetEcoute.sin_family = AF_INET;
    memset( &(_adresseInternetEcoute.sin_zero), 0, 8 );

    // if(_adresseInternetEcoute.sin_addr.s_addr <= 0) return false;

    std::cout << "Binding." << std::endl;
    // binding
    if( bind(_sockfdEcoute, (sockaddr*)&_adresseInternetEcoute, sizeof(_adresseInternetEcoute)) < 0 ) return false;

    std::cout << "Listening." << std::endl;
    // le socket écoute un port de la machine
    if( listen(_sockfdEcoute, _maxConnection) < 0) return false;
    return true;
}

bool Client::listener(int port, std::string protocole, std::mutex* locker, bool* stop, std::vector<std::string>* incomingDataQueue)
{
    std::cout << "Initialisation ..." << std::endl;
    if( !this->listenerInit(port, protocole) )
    {
        std::cout << "Error : cant init listener !" << std::endl;
        *stop = true;
        return false;
    }
    std::cout << "Initialisation finished" << std::endl;
    std::cout << "Listener ready" << std::endl;
    std::cout << "Listening ..." << std::endl;

    while(!*stop)
    {
        FD_ZERO(&_fdset);
        FD_SET(_sockfdEcoute, &_fdset);

        int max_socket = _sockfdEcoute;

        for(int i(0); i < _maxConnection; i++)
        {
            if(!_connections[i].estLibre)
            {
                if(_connections[i].sockfd > max_socket) max_socket = _connections[i].sockfd;
                FD_SET(_connections[i].sockfd, &_fdset);
            }
        }

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        select(max_socket + 1, &_fdset, NULL, NULL, &tv);

        if(FD_ISSET(_sockfdEcoute, &_fdset))
        {
            std::cout << "Oh un truc" << std::endl;
            int adresseLongueur = sizeof(_adresseInternetEcoute);
            int new_socket = accept(_sockfdEcoute, (sockaddr*)&_adresseInternetEcoute, &adresseLongueur);
            std::cout << "Le truc est toujours vivant..." << std::endl;
            
            // trouve un index libre dans _connections.
            // La case est libre lorsque la propriété isFree est true.
            int caseLibre = -1;
            for(int i(0); i < _maxConnection; i++)
            {
                if(_connections[i].estLibre)
                {
                    caseLibre = i;
                    break;
                }
            }
            if(caseLibre == -1)
            {
                shutdown(new_socket, 2);
            }
            else
            {
                _connections[caseLibre].estLibre = false;
                _connections[caseLibre].sockfd = new_socket;
            }
            std::cout << "La nouvelle case : " << caseLibre << std::endl;
        }

        for(int i(0); i < _maxConnection; i++)
        {
            if(FD_ISSET(_connections[i].sockfd, &_fdset))
            {
                buffer = new char[bufferSize];
                int bytesNumber = recv(_connections[i].sockfd, buffer, bufferSize, 0);

                if(bytesNumber == 0)
                {
                    _connections[i].estLibre = true;
                }
                else if(bytesNumber > 0)
                {
                    buffer[bytesNumber] = '\0';
                    locker->lock();
                    incomingDataQueue->push_back(buffer);
                    locker->unlock();
                }
                else
                {
                    _connections[i].estLibre = true;
                    _connections[i].sockfd = 0;
                }

                delete[] buffer; buffer = 0;
            }
        }
    }
    std::cout << "Exiting listener" << std::endl;
    return true;
}

std::thread Client::listenerSpawnThread(int port, std::string protocole, std::mutex* locker, bool* stop, std::vector<std::string>* incomingDataQueue)
{
    return std::thread(&listener, this, port, protocole, locker, stop, incomingDataQueue);
}

void Client::test()
{
    Sleep(5000);
}

bool Client::initWsa()
{
    WSADATA ws;
    if( WSAStartup(MAKEWORD(2, 2), &ws) < 0 ) return false;
    return true;
}

bool Client::initAdresseInternet(sockaddr_in &addr, std::string adresseCible, int port)
{
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;   // AF_INET est utilisé pour les protocole TCP et UDP
    memset( &(addr.sin_zero), 0, 8 );
    addr.sin_addr.s_addr = inet_addr( adresseCible.c_str() ); // Indique l'adresse du serveur

    if(addr.sin_addr.s_addr <= 0) return false;
    return true;
}

bool Client::initSocket(std::string protocole, int &sockfd)
{
    sockfd= protocole == "tcp" ? socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) :
            protocole == "udp" ? socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) :
            -1;

    if(sockfd < 0) return false;
    return true;
}

int Client::connection(std::string adresseServeur, int port, std::string protocole)
{
    // trouve un index libre dans _connections.
    // La case est libre lorsque la propriété isFree est true.
    int caseLibre;
    for(int i(0); i < _maxConnection; i++)
    {
        if(_connections[i].estLibre)
        {
            caseLibre = i;
            break;
        }

        if(i == _maxConnection - 1) return -4;
    }
    _connections[caseLibre].estLibre = false;

    // configuration des élements de WinSock2.h
    if( !this->initAdresseInternet(_connections[caseLibre].adressesInternet, adresseServeur, port) ) return -2;
    if( !this->initSocket(protocole, _connections[caseLibre].sockfd) ) return -3;

    // connection à la cible
    if( connect(_connections[caseLibre].sockfd, (sockaddr*)&_connections[caseLibre].adressesInternet, sizeof(_connections[caseLibre].adressesInternet)) < 0)
    return -1;

    return caseLibre;
}

bool Client::terminerConnection(int idConnection)
{
    if(idConnection < 0 || idConnection > _maxConnection) return false;
    else if(!_connections[idConnection].estLibre)
    {
        shutdown(_connections[idConnection].sockfd, 2);
        _connections[idConnection].estLibre = true;
        return true;
    }
    else return false;
}

void Client::envoyerText(int idConnection, std::string text)
{
    if(idConnection < 0 || idConnection > _maxConnection) return;
    else if(!_connections[idConnection].estLibre)
    {
        send(_connections[idConnection].sockfd, text.c_str(), text.size(), 0);
    }
}

std::string Client::recevoirText(int idConnection, int tempsMax)
{
    buffer[ recv( _connections[idConnection].sockfd, buffer, bufferSize, tempsMax ) ] = '\0';
    return buffer;
}

std::string Client::recevoirText(int idConnection)
{
    return this->recevoirText(idConnection, 0);
}