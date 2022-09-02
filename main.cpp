// Ce fichier est uniquement la pour illustré un exemple d'utilisation
#include <iostream>
#include "client.hpp"

int main(int argc, char **argv)
{
    Client client;
    int serveur = client.connection("127.0.0.1", 55000, "tcp");
    client.terminerConnection(serveur);
    return 0;
}