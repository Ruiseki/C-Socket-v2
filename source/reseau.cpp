#include <WinSock2.h>
#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <fstream>

#include "reseau.hpp"

Reseau::Reseau(int maxConnection, int tailleBuffer) : maxConnexion(maxConnection), bufferSize(tailleBuffer)
{
    connexions = new connexion[maxConnexion];
}

Reseau::Reseau(int maxConnection) : Reseau(maxConnection, 128)
{
    
}

Reseau::Reseau() : Reseau(4, 128)
{ }

Reseau::~Reseau()
{
    delete[] connexions;
}


int Reseau::connection(std::string adresseServeur, int port, std::string protocole)
{
    // trouve un index libre dans connexions.
    // La case est libre lorsque la propriété estLibre est true.
    int caseLibre;
    for(int i(0); i < maxConnexion; i++)
    {
        if(connexions[i].estLibre)
        {
            caseLibre = i;
            break;
        }

        if(i == maxConnexion - 1) return -5;
    }
    connexions[caseLibre].estLibre = false;
    
    int codeErreur = 0;
    // configuration des élements de WinSock2.h
    if( !this->initWsa() ) codeErreur = -4;
    if( codeErreur == 0 && !this->initAdresseSocket(connexions[caseLibre].adresseSocket, adresseServeur, port) ) codeErreur = -2;
    if( codeErreur == 0 && !this->initSocket(protocole, connexions[caseLibre].sockfd) ) codeErreur = -3;

    // connection à la cible
    if( codeErreur == 0)
    {
        this->wlog("---- CONNECTING...");
        if( connect(connexions[caseLibre].sockfd, (sockaddr*)&connexions[caseLibre].adresseSocket, sizeof(connexions[caseLibre].adresseSocket)) < 0)
            codeErreur = -1;
        else this->wlog("DONE");
    }

    if(codeErreur == 0) return caseLibre;
    else
    {
        WSACleanup();
        if(codeErreur == -1)
        {
            closesocket(connexions[caseLibre].sockfd);
            this->wlog("CONNECTION TIMEOUT");
        }

        connexions[caseLibre].estLibre = true;
        return codeErreur;
    }
}

bool Reseau::terminerConnection(int idConnexion)
{
    this->wlog("---- DISCTONNECTING...");

    if(idConnexion < 0 || idConnexion > maxConnexion)
    {
        this->wlog("ERROR : WRONG CONNECTION ID");
        return false;
    }
    else if(!connexions[idConnexion].estLibre)
    {
        shutdown(connexions[idConnexion].sockfd, 2);
        closesocket(connexions[idConnexion].sockfd);
        connexions[idConnexion].estLibre = true;
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

void Reseau::listener(int port, std::string protocole)
{
    // Initialisation des variable, bind() puis listen().
    if( !this->listenerInit(port, protocole) )
    {
        this->wlog("--- LISTENER INIT FAILED");
        std::this_thread::sleep_for(std::chrono::seconds(3));
        stopListener = true;
        return;
    }

    while(!stopListener)
    {
        // remise a zéro de la liste de file descriptor
        FD_ZERO(&fdset);

        // ajout du socket serveur
        FD_SET(_sockfdEcoute, &fdset);

        // max_socket est utilisé pour select(), le premier argument étant le socket avec la valeur la plus élevé + 1
        int max_socket = _sockfdEcoute;

        // On aimerait ajouter à la liste de file descriptor tous les socket des clients connecter
        for(int i(0); i < maxConnexion; i++)
        {
            if(!connexions[i].estLibre)
            {
                if(connexions[i].sockfd > max_socket) max_socket = connexions[i].sockfd;
                // ajout d'un socket client
                FD_SET(connexions[i].sockfd, &fdset);
            }
        }

        // variable initilisé pour le timeout de select();
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        // select() permet au programme de manipulé plusieurs file descriptor en même temps. En l'occurence, ceux du serveur et des autre clients
        select(max_socket + 1, &fdset, NULL, NULL, &tv);

        // On regarde si il y a de l'activité sur le socket seveur. Si oui, c'est forcément une demande de connection entrante
        if(FD_ISSET(_sockfdEcoute, &fdset))
        {
            int adresseLongueur = sizeof(adresseSocketEcoute);
            int new_socket = accept(_sockfdEcoute, (sockaddr*)&adresseSocketEcoute, &adresseLongueur);
            
            // trouve un index libre dans connexions.
            // La case est libre lorsque la propriété isFree est true.
            int caseLibre = -1;
            for(int i(0); i < maxConnexion; i++)
            {
                if(connexions[i].estLibre)
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
                connexions[caseLibre].estLibre = false;
                connexions[caseLibre].sockfd = new_socket;
            }
        }

        // Pour chaque élements de la variable permettant de sauvegarder les connections...
        for(int i(0); i < maxConnexion; i++)
        {
            // On regarde si un client est attribué, puis on regarde si son socket a reçus quelque chose
            if(!connexions[i].estLibre && FD_ISSET(connexions[i].sockfd, &fdset))
            {
                // allocation du buffer
                buffer = new char[bufferSize];
                int bytesNumber = recv(connexions[i].sockfd, buffer, bufferSize, 0);

                // Le client se déconnecte
                if(bytesNumber == 0)
                {
                    connexions[i].estLibre = true;
                    connexions[i].sockfd = 0;
                }
                // Reception des données
                else if(bytesNumber > 0)
                {
                    buffer[bytesNumber] = '\0';
                    // les données sont envoyées dans la file d'attente
                    locker.lock();
                    dataQueue.push_back( (std::string)buffer );
                    locker.unlock();
                }
                // Quelque chose ne se passe pas comme prévus
                else
                {
                    connexions[i].estLibre = true;
                    connexions[i].sockfd = 0;
                }

                // déallocation du buffer
                delete[] buffer; buffer = 0;
            }
        }
    }
    closesocket(_sockfdEcoute);
    WSACleanup();
}

std::thread Reseau::listenerSpawnThread(int port, std::string protocole)
{
    this->wlog("---- SPAWNING LISTENER THREAD");
    return std::thread(&listener, this, port, protocole);
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
        std::cout << "BINDING ERROR : " << WSAGetLastError() << std::endl << "The port may be occupied by another program" << std::endl;
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

void Reseau::envoyerText(int idConnexion, std::string text)
{
    if(idConnexion < 0 || idConnexion > maxConnexion) return;
    else if(!connexions[idConnexion].estLibre)
    {
        send(connexions[idConnexion].sockfd, text.c_str(), text.size(), 0);
    }
}

void Reseau::envoyerBinaire(int idConnexion, char donnees[], int tailleBufferDonnees)
{
    if(idConnexion < 0 || idConnexion > maxConnexion) return;
    else if(!connexions[idConnexion].estLibre)
    {
        send(connexions[idConnexion].sockfd, donnees, tailleBufferDonnees, 0);
    }
}

void Reseau::wlog(std::string logMessage)
{
    std::ofstream wlog("./log.txt", std::ios::app);
    wlog << logMessage << std::endl;
}