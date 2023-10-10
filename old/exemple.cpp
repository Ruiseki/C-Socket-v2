#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <fstream>

#include "WinSock2.h"
#include "windows.h"
#include "client.hpp"

using namespace std;

struct Config
{
    string addr;
    string username;
    int portTalk;
    int portListen;
};

void updateConfigFile(Config conf)
{
    ofstream configFile("./config.conf");
    if(configFile)
    {
        configFile << conf.addr << endl;
        configFile << conf.username << endl;
        configFile << conf.portTalk << endl;
        configFile << conf.portListen;
    }
}

struct Config configFile()
{
    Config conf;
    ifstream configFile("./config.conf");

    if(configFile)
    {
        string line;
        try
        {
            getline(configFile, line); conf.addr = line;
            getline(configFile, line); conf.username = line;
            getline(configFile, line); conf.portTalk = stoi(line);
            getline(configFile, line); conf.portListen = stoi(line);
        }
        catch(const std::exception& e) { }
        
    }
    else
    {
        conf.addr = "127.0.0.1";
        conf.username = "Guest";
        conf.portTalk = 55000;
        conf.portListen = 55000;
        updateConfigFile(conf);
    }

    return conf;
}

void envoyerFichier(Client* client, int idConnexion)
{
    ifstream fichier;
    string cheminInput, vraiChemin, nomFichier;
    do
    {
        system("cls");
        cout << "Chemin du fichier : ";
        vraiChemin = "";
        nomFichier = "";
        getline(cin, cheminInput);
        for(auto &letter : cheminInput)
        {
            if(letter == '\\') vraiChemin += '/';
            else if(letter != '"' && letter != '\'') vraiChemin += letter;

            if(letter == '\\' || letter == '/') nomFichier = "";
            else if(letter != '\"')nomFichier += letter;
        }
        fichier.open(vraiChemin);
    } while(!fichier);
    fichier.close();
    client->envoyerText(idConnexion, nomFichier);
    client->envoyerFichier(idConnexion, vraiChemin);
    system("cls");
    cout << "[ done ]" << endl << endl;
}

void recevoirFichier(Client* client, int idConnexion)
{
    system("cls");
    cout << "[ recieving file ... ]" << endl;
    string nomFichier = client->recevoirText(idConnexion, );
    cout << "[ downloading ... ]" << endl;
    client->recevoirFichier(idConnexion, "./"+nomFichier);
    system("cls");
    cout << "[ done ]" << endl << endl;
}

void recupererMessage(Client* client, vector<Client::messageInfo>* dataQueue, bool* stop, mutex* locker)
{
    while(!*stop)
    {
        if(dataQueue->size() > 0)
        {
            locker->lock();
            if(dataQueue->size() == 0)
            {
                locker->unlock();
                continue;
            }
            for(Client::messageInfo &element : *dataQueue)
            {
                if(element.message.substr(0, 3) == "-//") element.message.erase(element.message.begin());
                else if(element.message.substr(0, 2) == "//")
                {
                    if(element.message == "//recv")
                    {
                        recevoirFichier(client, element.idConnexion);
                        dataQueue->clear();
                    }
                    else if(element.message.substr(0, 7) == "//name:") client->setNomConnection(element.idConnexion, element.message.substr(7, element.message.size()));
                }
                else cout << element.nom << " -> " << element.message << endl;
            }
            dataQueue->clear();
            locker->unlock();
        }
    }
}

void connection(Client* client, int port, int portListen, string target, string username)
{
    system("cls");
    cout << "Connecting to " << target << " ..." << endl;
    vector<Client::messageInfo> dataQueue;
    bool stop = false;
    mutex locker;

    thread listener = client->listenerSpawnThread(portListen, "tcp", &locker, &stop, &dataQueue);

    int idConnexion = client->connection(target, port, "tcp");

    if(idConnexion < 0)
    {
        stop = true;
        cout << "Erreur (" << idConnexion << ")" << endl;
        listener.join();
        this_thread::sleep_for(chrono::seconds(3));
        return;
    }

    cout << "Connection successful" << endl;

    thread message(recupererMessage, client, &dataQueue, &stop, &locker);
    client->envoyerText("//name:"+username);
    cin.ignore();
    string input;
    do
    {
        getline(cin, input);
        if(input == "//close")
        {
            client->terminerConnection(idConnexion);
            stop = true;
        }
        else if(input == "//send")
        {
            client->envoyerText(idConnexion, "//recv");
            envoyerFichier(client, idConnexion);
        }
        else
        {
            if(input.substr(0, 2) == "//") input.insert(input.begin(), '-');
            client->envoyerText(idConnexion, input);
        }
    } while(!stop);

    cout << "Stopping thread ..." << endl;
    listener.join();
    message.join();
}

int main()
{
    Client *client;
    client = new Client;
    Config conf = configFile();
    bool exit = false;
    int selection;

    do
    {
        updateConfigFile(conf);
        system("cls");
        cout << "Utilisateur -> " << conf.username << endl << endl;
        cout << "DESTINATION :" << endl;
        cout << "Adresse : " << conf.addr << endl;
        cout << "Port : " << conf.portTalk << endl << endl;
        cout << "HOTE :" << endl;
        cout << "Port d'ecoute : " << conf.portListen << endl;
        cout << endl;
        cout << "1: connection" << endl;
        cout << "2: change name" << endl;
        cout << "3: changer IP" << endl;
        cout << "4: changer port" << endl;
        cout << "5: changer port d'ecoute" << endl;
        cout << "6: quitter" << endl;

        cin >> selection;

        switch (selection)
        {
        case 1:
            connection(client, conf.portTalk, conf.portListen, conf.addr, conf.username);
            system("cls");
            break;
        case 2:
            system("cls");
            cout << "Nouveau nom : ";
            cin.ignore();
            getline(cin, conf.username);
            break;
        case 3:
            system("cls");
            cout << "Nouvelle adresse : ";
            cin >> conf.addr;
            break;
        case 4:
            system("cls");
            cout << "Nouveau port : ";
            cin >> conf.portTalk;
            break;
        case 5:
            system("cls");
            cout << "Nouveau port : ";
            cin >> conf.portListen;
            break;
        case 6:
            system("cls");
            exit = true;
            break;
        default:
            break;
        }
    } while(!exit);

    delete client;
    return 0;
}