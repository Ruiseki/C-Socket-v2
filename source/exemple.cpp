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
    int portTalk;
    int portListen;
};

void updateConfigFile(Config conf)
{
    ofstream configFile("./config.conf");
    if(configFile)
    {
        configFile << conf.addr << endl;
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
            getline(configFile, line); conf.portTalk = stoi(line);
            getline(configFile, line); conf.portListen = stoi(line);
        }
        catch(const std::exception& e) { }
        
    }
    else
    {
        conf.addr = "127.0.0.1";
        conf.portTalk = 55000;
        conf.portListen = 55000;
        updateConfigFile(conf);
    }

    return conf;
}

void checkMessage(bool* stop, mutex* locker, vector<string>* dataQueue)
{
    do
    {
        if(dataQueue->size() > 0)
        {
            locker->lock();

            for(string message : *dataQueue)
            {
                cout << "-> " << message << endl;
            }
            dataQueue->clear();

            locker->unlock();
        }
    } while (!*stop);
}

void creerReseau(Client client, Config conf)
{
    mutex locker;
    bool stop = false;
    vector<string> dataQueue;
    thread listener = client.listenerSpawnThread(conf.portListen, "tcp", &locker, &stop, &dataQueue);

    cout << "Server has started on port " << conf.portListen << endl;
    thread messageChecker(&checkMessage, &stop, &locker, &dataQueue);

    string entry;
    do
    {
        getline(cin, entry);
        if(entry != "//close") ;// envoyer le message aux clients;

    } while(entry != "//close");
    stop = true;
    messageChecker.join();
    listener.join();
}

void connection(Client client, Config conf)
{
    cout << "Waiting to connect..." << endl;
    int idConnection = client.connection(conf.addr, conf.portTalk, "tcp");

    if(idConnection < 0)
    {
        cout << "Error ! (" << idConnection << ")" << endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        return;
    }

    cout << "Connected !" << endl;

    string entry;
    vector<string> dataQueue;
    bool stop = false;
    mutex locker;

    thread messageChecker(&checkMessage, &stop, &locker, &dataQueue);

    do
    {
        getline(cin, entry);
        if(entry != "//close") client.envoyerText(idConnection, entry);
    } while (entry != "//close");
    stop = true;
    messageChecker.join();
    client.terminerConnection(idConnection);
}

int main()
{
    Client client;
    Config conf = configFile();
    bool exit = false;
    int selection;

    do
    {
        updateConfigFile(conf);
        system("cls");
        cout << "DESTINATION :" << endl;
        cout << "Adresse : " << conf.addr << endl;
        cout << "Port : " << conf.portTalk << endl << endl;
        cout << "HOTE :" << endl;
        cout << "Port d'ecoute : " << conf.portListen << endl;
        cout << endl;
        cout << "1: creer un reseau" << endl;
        cout << "2: se connecter" << endl << endl;
        cout << "3: changer IP" << endl;
        cout << "4: changer port" << endl;
        cout << "5: changer port d'ecoute" << endl;
        cout << "6: quitter" << endl;

        cin >> selection;

        switch (selection)
        {
        case 1:
            system("cls");
            creerReseau(client, conf);
            break;
        case 2:
            system("cls");
            connection(client, conf);
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

    return 0;
}