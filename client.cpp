#include "client.hpp"
#include <WinSock2.h>
#include <string>
#include <thread>
#include <fstream>

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
    // Déclaration du socket d'écoute
    _sockfdEcoute = protocole == "tcp" ? socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) :
                       protocole == "udp" ? socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) :
                       -1;

    if(_sockfdEcoute < 0) return false;

    // Déclaration addr écoute
    sockaddr_in adresseInternetEcoute;
    adresseInternetEcoute.sin_addr.s_addr = INADDR_ANY;
    adresseInternetEcoute.sin_port = htons(port);
    adresseInternetEcoute.sin_family = AF_INET;
    memset( &(adresseInternetEcoute.sin_zero), 0, 8 );

    if(adresseInternetEcoute.sin_addr.s_addr <= 0) return false;

    // binding
    if( bind(_sockfdEcoute, (sockaddr*)&adresseInternetEcoute, sizeof(adresseInternetEcoute)) < 0 ) return false;

    // le socket écoute un port de la machine
    if( listen(_sockfdEcoute, _maxConnection) < 0) return false;

    // réinitialisation
    FD_ZERO(&_fdset);

    // ajout d'un file descriptor dans la liste
    FD_SET(_sockfdEcoute, &_fdset);
}

void Client::listener(int port, std::string protocole)
{
    this->listenerInit(port, protocole);
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