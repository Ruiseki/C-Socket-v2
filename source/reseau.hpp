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

    virtual std::thread listenerSpawnThread(int port, std::string protocole) = 0;

protected:

    virtual void listener(int port, std::string protocole) = 0;

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
     * @brief Envoi une chaine de caractere a une connection
     * @param idConnection la connection cible (voir connection() )
     * @param text La chaine de caractere a envoyer
     * @return void
     */
    void envoyerText(int idConnection, std::string text);

    /*
     * @brief Envoi une chaine de caractere a tous les clients stocker dans connexions[]
     * @param text La chaine de caractere a envoyer
     * @return void
     */
    void envoyerText(std::string text);

    /*
     * @brief donne un nom a une connection
     * @param idConnection la connection cible (voir connection() )
     * @param nom le nom de la connection
     */
    void setNomConnection(int idConnection, std::string nom);

    void wlog(std::string logMessage);

    struct messageInfo
    {
        int idConnection;
        std::string nom;
        std::string message;
    };
    std::vector<messageInfo> dataQueue;

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
    std::mutex locker;
    bool stopListener = false;

    WSADATA wsaData;
    // fdset est une liste de file descriptor. Il permet de manipuler ces derniers
    struct fd_set fdset;
    int _sockfdEcoute;
    sockaddr_in adresseSocketEcoute;
};

#endif // !RESEAU_HPP
