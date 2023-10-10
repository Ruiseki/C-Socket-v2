#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <WinSock2.h>
#include <string>
#include <vector>
#include <mutex>
#include <thread>

#include "reseau.hpp"

class Client : public Reseau
{
public:
    Client();
    ~Client();

    /* 
     * @brief Connection avec un client ou un serveur
     * @param adresseCible Adresse IP de la destination
     * @param port Port utilisé pour la communication
     * @param protocole Protocole utilisé. tcp / udp
     * @return Int. -1 timeout, 
     * -2 erreur lors de l'initialisation de sockaddr_in, 
     * -3 erreur lors de l'initialisation du socket, 
     * -4 erreur lors de l'initialisation des WSA, 
     * -5 quantité maximum de connections simulatées atteinte.
     * Sinon, l'identifiant de la connection crée (entier supérieur ou égale à zéro)
     */
    int connection(std::string adresseCible, int port, std::string protocole);
    
    /*
     * @brief Permet d'écouter sur un port. Les données reçus seront pusher dans le vector dataQueue. Mettre le boolean stop a true permet de stoper le thread
     * @param port Port d'écoute
     * @param protocole Protocole utilisé. tcp / udp
     * @param locker std::mutex permettant de bloquer une valeur partagé pour la lire et l'écrire sans quel ne soit modifié en même temps
     * @param stop bool pour stoper le thread
     * @param dataQueue Les donnée reçus sur ce port serons stocker dans ce vector
     * @return Le thread invoqué
     */
    std::thread listenerSpawnThread(int port, std::string protocole);

    /*
     * @brief Termine une connection existante.
     * @param identifiant de la connection (voir connection() )
     * @return false si la connection n'existe pas, true si la connection existe
     */
    bool terminerConnection(int idConnection);

private:

    /*
     * @brief Le listener serveur en lui même. Voir Client::listenerSpawnThread
     */
    void listener(int port, std::string protocole);
};

#endif