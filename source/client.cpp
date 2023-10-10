#include "client.hpp"
#include <WinSock2.h>
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <thread>
#include <iostream>

Client::Client()
{ }

Client::~Client()
{
    
}

void Client::listener(int port, std::string protocole)
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
                    messageInfo info;
                    info.idConnection = i;
                    info.message = (std::string)buffer;
                    info.nom = connexions[i].nom;
                    dataQueue.push_back( info );
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

std::thread Client::listenerSpawnThread(int port, std::string protocole)
{
    this->wlog("---- SPAWNING LISTENER THREAD");
    return std::thread(&listener, this, port, protocole);
}

int Client::connection(std::string adresseServeur, int port, std::string protocole)
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

bool Client::terminerConnection(int idConnection)
{
    this->wlog("---- DISCTONNECTING...");

    if(idConnection < 0 || idConnection > maxConnexion)
    {
        this->wlog("ERROR : WRONG CONNECTION ID");
        return false;
    }
    else if(!connexions[idConnection].estLibre)
    {
        shutdown(connexions[idConnection].sockfd, 2);
        closesocket(connexions[idConnection].sockfd);
        connexions[idConnection].estLibre = true;
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