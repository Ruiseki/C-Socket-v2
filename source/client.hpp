#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <WinSock2.h>
#include <string>
#include <fstream>
#include <vector>
#include <mutex>
#include <thread>

class Client
{
public:
    /*
     * @param maxConnection Defini le nombre maximum de connection simultané. Par défaut 4.
     * @return Détruit le client si la variable WSA n'a pas pus être initialisé.
     * Le client serait inutilisable
     */
    Client(int maxConnection, int tailleBuffer, int tailleBufferDonnee);
    Client(int maxConnection);
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
     * -4 quantité maximum de connections simulatées atteinte.
     * Sinon, l'identifiant de la connection crée (entier positif non nul).
     */
    int connection(std::string adresseCible, int port, std::string protocole);
    

    /*
     * @brief Permet d'écouter sur un port. Les données reçus seront pusher dans le vector dataQueue. Mettre le boolean stop a true permet de stoper le thread
     * @param port Port d'écoute
     * @param protocole Protocole utilisé. tcp / udp
     * @param locker std::mutex permettant de bloquer une valeur partagé pour la lire et l'écrire sans quel ne soit modifié en même temps
     * @param stop bool pour stoper le thread
     * @param dataQueue Les donnée reçus sur ce port serons stocker dans ce vector
     * @return Le thread alors invoqué.
     */
    std::thread listenerSpawnThread(int port, std::string protocole, std::mutex* locker, bool* stop, std::vector<std::string>* dataQueue);

    /*
     * @brief Termine une connection existante. 
     * @param identifiant de la connection (voir connection() )
     * @return false si la connection n'existe pas, true si la connection existe
     */
    bool terminerConnection(int idConnection);

    /*
     * @brief Envoi une chaîne de caractère à un connection
     * @param idConnection la connection cible (voir connection() )
     * @param text La chaîne de caractère a envoyer
     * @return void
     */
    void envoyerText(int idConnection, std::string text);

    /*
     * @brief Attend une chaîne de caractère de la cible
     * @param idConnection la connection cible (voir connection() )
     * @return void
     */
    std::string recevoirText(int idConnection);

    /*
     * @todo coder la méthode
     * @brief Attend une chaîne de caractère de la cible
     * @param idConnection la connection cible (voir connection() )
     * @param tempsMax le temps avant de timed out
     * @return void
     */
    std::string recevoirText(int idConnection, int tempsMax);

    /*
     * @todo coder la méthode
     * @brief Envoi un fichiers a une cible
     * @param idConnection la connection cible (voir connection() )
     * @param chemin le chemin du fichier a envoyer
     * @return void
     */
    void envoyerFichier(int idConnection, std::string chemin);
    /*
     * @todo coder la méthode
     * @brief Recoie un fichiers d'un cible
     * @param idConnection la connection cible (voir connection() )
     * @param chemin le chemin du fichier a enregistrer
     * @return void
     */
    void recevoirFichier(int idConnection, std::string chemin);
    /*
     * @todo coder la méthode
     * @brief Recoie un fichiers d'un cible
     * @param idConnection la connection cible (voir connection() )
     * @param chemin le chemin du fichier a enregistrer
     * @param tempsMax le temps avant de timed out
     * @return void
     */
    void recevoirFichier(int idConnection, std::string chemin, int tempsMax);

private:
    /*
     * @brief initialisation de la structure donnant des infos sur le socket de windows
     * @return false pour un echec d'initialisation, true pour une réussite
     */
    bool initWsa();

    /*
     * @brief Initialisation de la structure sockaddr_in de WinSock2.h
     * @param addr Adresse internet de la cible. Trouvable dans _connections[].adresseSocket
     * @param adresseCible Adresse IP de la destination
     * @param port Port utilisé pour la communication
     * @return False pour un echec d'initialisation, true pour une réussite
     */
    bool initAdresseSocket(sockaddr_in &addr, std::string adresseCible, int port);

    /*
     * @brief Initialisation du socket utilisé pour la communication
     * @param protocole Protocole utilisé. tcp / udp
     * @return False pour un echec d'initialisation, true pour une réussite
     */
    bool initSocket(std::string protocole, int &sockfd);

    /*
     * @brief Initialise les variable nessessaire pour l'écoute. Comme _socketEcoute et _adresseSocketEcoute, bind() et listen().
     * @return false en cas d'erreur d'initialisation
     */
    bool listenerInit(int port, std::string protocole);

    /*
     * @brief le listener en lui même. Voir Client::listenerSpawnThread
     */
    void listener(int port, std::string protocole, std::mutex* locker, bool* stop, std::vector<std::string>* incomingDataQueue);
    
    // Le nombre maximal de connection simultané
    int const _maxConnection;
    // Structure définissant une connection
    struct _connection
    {
        sockaddr_in adresseSocket;
        bool estLibre = true;
        int sockfd = 0;
    };
    // Tableau contenant les connections
    _connection* _connections;
    char* buffer, dataBuffer;
    int bufferSize, dataBufferSize;
    
    WSADATA wsaData;
    // fdset est une liste de file descriptor (au sens littérale du terme). Il permet de manipulé ces derniers
    struct fd_set _fdset;
    // socket d'écoute du client
    int _sockfdEcoute;
    sockaddr_in _adresseSocketEcoute;
};

#endif