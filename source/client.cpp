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
    _connections = new _connection[_maxConnection];
}

Client::Client(int maxConnection) : Client(maxConnection, 128, 1024)
{ }

Client::Client() : Client(4, 128, 1024)
{ }

Client::~Client()
{
    delete[] _connections;
}

bool Client::listenerInit(int port, std::string protocole)
{
    if( !this->initWsa() ) return false;
    // Déclaration du socket d'écoute
    if( !this->initSocket(protocole, _sockfdEcoute) ) return false;
    // Déclaration addr écoute
    if( !this->initAdresseSocket(_adresseSocketEcoute, "aucune", port) ) return false;

    // binding
    int errorCode = bind(_sockfdEcoute, (sockaddr*)&_adresseSocketEcoute, sizeof(_adresseSocketEcoute));
    if( errorCode == SOCKET_ERROR )
    {
        std::cout << "BINDING ERROR : " << std::endl << WSAGetLastError() << std::endl;
        return false;
    }

    // le socket écoute un port de la machine
    if( listen(_sockfdEcoute, _maxConnection) < 0)
    {
        std::cout << "LISTEN ERROR" << std::endl;
        return false;
    }
    return true;
}

void Client::listener(int port, std::string protocole, std::mutex* locker, bool* stop, std::vector<std::string>* dataQueue)
{
    // Initialisation des variable, bind() puis listen().
    if( !this->listenerInit(port, protocole) )
    {
        this->wlog("--- LISTENER INIT FAILED");
        std::this_thread::sleep_for(std::chrono::seconds(3));
        *stop = true;
        return;
    }

    while(!*stop)
    {
        // remise a zéro de la liste de file descriptor
        FD_ZERO(&_fdset);

        // ajout du socket serveur
        FD_SET(_sockfdEcoute, &_fdset);

        // max_socket est utilisé pour select(), le premier argument étant le socket avec la valeur la plus élevé + 1
        int max_socket = _sockfdEcoute;

        // On aimerait ajouter à la liste de file descriptor tous les socket des clients connecter
        for(int i(0); i < _maxConnection; i++)
        {
            if(!_connections[i].estLibre)
            {
                if(_connections[i].sockfd > max_socket) max_socket = _connections[i].sockfd;
                // ajout d'un socket client
                FD_SET(_connections[i].sockfd, &_fdset);
            }
        }

        // variable initilisé pour le timeout de select();
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        // select() permet au programme de manipulé plusieurs file descriptor en même temps. En l'occurence, ceux du serveur et des autre clients
        select(max_socket + 1, &_fdset, NULL, NULL, &tv);

        // On regarde si il y a de l'activité sur le socket seveur. Si oui, c'est forcément une demande de connection entrante
        if(FD_ISSET(_sockfdEcoute, &_fdset))
        {
            int adresseLongueur = sizeof(_adresseSocketEcoute);
            int new_socket = accept(_sockfdEcoute, (sockaddr*)&_adresseSocketEcoute, &adresseLongueur);
            
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
            // Le serveur est plein
            if(caseLibre == -1)
            {
                shutdown(new_socket, 2);
            }
            // Place libre trouver, on sauvegarde la connection
            else
            {
                _connections[caseLibre].estLibre = false;
                _connections[caseLibre].sockfd = new_socket;
            }
        }

        // Pour chaque élements de la variable permettant de sauvegarder les connections...
        for(int i(0); i < _maxConnection; i++)
        {
            // On regarde si un client est attribué, puis on regarde si son socket a reçus quelque chose
            if(!_connections[i].estLibre && FD_ISSET(_connections[i].sockfd, &_fdset))
            {
                // allocation du buffer
                buffer = new char[bufferSize];
                int bytesNumber = recv(_connections[i].sockfd, buffer, bufferSize, 0);

                // Le client se déconnecte
                if(bytesNumber == 0)
                {
                    _connections[i].estLibre = true;
                    _connections[i].sockfd = 0;
                }
                // Reception des données
                else if(bytesNumber > 0)
                {
                    buffer[bytesNumber] = '\0';
                    // les données sont envoyées dans la file d'attente
                    locker->lock();
                    dataQueue->push_back( (std::string)buffer );
                    locker->unlock();
                }
                // Quelque chose ne se passe pas comme prévus
                else
                {
                    _connections[i].estLibre = true;
                    _connections[i].sockfd = 0;
                }

                // déallocation du buffer
                delete[] buffer; buffer = 0;
            }
        }
    }
    closesocket(_sockfdEcoute);
    WSACleanup();
}

std::thread Client::listenerSpawnThread(int port, std::string protocole, std::mutex* locker, bool* stop, std::vector<std::string>* dataQueue)
{
    this->wlog("---- SPAWNING LISTENER THREAD");
    return std::thread(&listener, this, port, protocole, locker, stop, dataQueue);
}

bool Client::initWsa()
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

bool Client::initAdresseSocket(sockaddr_in &addr, std::string adresseCible, int port)
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

bool Client::initSocket(std::string protocole, int &sockfd)
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

int Client::connection(std::string adresseServeur, int port, std::string protocole)
{
    // trouve un index libre dans _connections.
    // La case est libre lorsque la propriété estLibre est true.
    int caseLibre;
    for(int i(0); i < _maxConnection; i++)
    {
        if(_connections[i].estLibre)
        {
            caseLibre = i;
            break;
        }

        if(i == _maxConnection - 1) return -5;
    }
    _connections[caseLibre].estLibre = false;
    
    int codeErreur = 0;
    // configuration des élements de WinSock2.h
    if( !this->initWsa() ) codeErreur = -4;
    if( codeErreur == 0 && !this->initAdresseSocket(_connections[caseLibre].adresseSocket, adresseServeur, port) ) codeErreur = -2;
    if( codeErreur == 0 && !this->initSocket(protocole, _connections[caseLibre].sockfd) ) codeErreur = -3;

    // connection à la cible
    if( codeErreur == 0)
    {
        this->wlog("---- CONNECTING...");
        if( connect(_connections[caseLibre].sockfd, (sockaddr*)&_connections[caseLibre].adresseSocket, sizeof(_connections[caseLibre].adresseSocket)) < 0)
            codeErreur = -1;
        else this->wlog("DONE");
    }

    if(codeErreur == 0) return caseLibre;
    else
    {
        WSACleanup();
        if(codeErreur == -1)
        {
            closesocket(_connections[caseLibre].sockfd);
            this->wlog("CONNECTION TIMEOUT");
        }

        _connections[caseLibre].estLibre = true;
        return codeErreur;
    }
}

bool Client::terminerConnection(int idConnection)
{
    this->wlog("---- DISCTONNECTING...");

    if(idConnection < 0 || idConnection > _maxConnection)
    {
        this->wlog("ERROR : WRONG CONNECTION ID");
        return false;
    }
    else if(!_connections[idConnection].estLibre)
    {
        shutdown(_connections[idConnection].sockfd, 2);
        closesocket(_connections[idConnection].sockfd);
        _connections[idConnection].estLibre = true;
        WSACleanup();
        this->wlog("DONE");
        return true;
    }
    else
    {
        this->wlog("ERROR : INCOHERANCE");
        return false;
    }
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


bool Client::envoyerFichier(int idConnection, std::string chemin)
{
    return true;
}

bool Client::recevoirFichier(int idConnection, std::string chemin)
{
    return true;
}

void Client::wlog(std::string logMessage)
{
    std::ofstream wlog("./log.txt", std::ios::app);
    wlog << logMessage << std::endl;
}