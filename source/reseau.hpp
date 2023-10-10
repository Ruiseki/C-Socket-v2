#ifndef RESEAU_HPP
#define RESEAU_HPP

#include <WinSock2.h>
#include <vector>
#include <string>
#include <mutex>
#include <thread>

class Reseau
{
public:
    /*
     * @param maxConnection Defini le nombre maximum de connection simultané. Par défaut 4.
     * @return Détruit le l'objet si la variable WSA n'a pas pus être initialisé.
     */
    Reseau(int maxConnection, int tailleBuffer, int tailleBufferDonnee);
    Reseau(int maxConnection);
	Reseau();
	~Reseau();

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
     * @brief Termine une connection existante.
     * @param identifiant de la connection (voir connection() )
     * @return false si la connection n'existe pas, true si la connection existe
     */
    bool terminerConnection(int idConnexion);

    /*
     * @brief Permet d'écouter sur un port. Les données reçus seront pusher dans le vector dataQueue. Mettre le boolean stopListener a true permet de stoper le thread
     * @param port Port d'écoute
     * @param protocole Protocole utilisé. tcp / udp
     * @return Le thread invoqué
     */
    std::thread listenerSpawnThread(int port, std::string protocole);

    /*
     * @brief Envoi une chaine de caractere a une connection
     * @param idConnexion la connection cible (voir connection() )
     * @param text La chaine de caractere a envoyer
     * @return void
     */
    void envoyerText(int idConnexion, std::string text);

    struct messageInfo
    {
        int idConnexion;
        std::string message;
        std::string nom;
        std::string destinataire;
    };
    std::vector<messageInfo> dataQueue;
    std::mutex locker;
    bool stopListener = false;

protected:

    /*
     * @brief Le listener serveur en lui même. Voir Client::listenerSpawnThread
     */
    void listener(int port, std::string protocole);

    /*
     * @brief Initialise les variable nessessaire pour l'écoute. Comme _socketEcoute et adresseSocketEcoute, bind() et listen().
     * @return false en cas d'erreur d'initialisation
     */
    bool listenerInit(int port, std::string protocole);

    /*
	 * @brief initialisation de la structure donnant des infos sur le socket de windows
	 * @return false pour un echec d'initialisation, true pour une reussite
	 */
	bool initWsa();

    /*
     * @brief Initialisation de la structure sockaddr_in de WinSock2.h
     * @param addr Adresse internet de la cible. Trouvable dans connexions[].adresseSocket
     * @param adresseCible Adresse IP de la destination. Renseigner "aucune" pour une configuration serveur
     * @param port Port utilise pour la communication
     * @return False pour un echec d'initialisation, true pour une reussite
     */
    bool initAdresseSocket(sockaddr_in& addr, std::string adresseCible, int port);

    /*
     * @brief Initialisation du socket utilise pour la communication
     * @param protocole Protocole utilise. tcp / udp
     * @return False pour un echec d'initialisation, true pour une reussite
     */
    bool initSocket(std::string protocole, int& sockfd);

    /*
     * @brief donne un nom a une connection
     * @param idConnexion la connection cible (voir connection() )
     * @param nom le nom de la connection
     */
    void setNomConnection(int idConnexion, std::string nom);

    void wlog(std::string logMessage);

    int const maxConnexion;
    struct connexion
    {
        sockaddr_in adresseSocket;
        bool estLibre = true;
        int sockfd = 0;
        std::string nom;
    };
    connexion* connexions;
    char* buffer, * dataBuffer;
    int bufferSize, dataBufferSize;

    WSADATA wsaData;
    // fdset est une liste de file descriptor. Il permet de manipuler ces derniers
    struct fd_set fdset;
    int _sockfdEcoute;
    sockaddr_in adresseSocketEcoute;
};

#endif // !RESEAU_HPP
