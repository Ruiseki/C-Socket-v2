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

void affichage(mutex* locker, string ip, bool* stop, vector<string>* dataQueue)
{
    while(!*stop)
    {
        if(dataQueue->size() > 0)
        {
            locker->lock();
            for(string content : *dataQueue)
            {
                cout << ip << " : " << content << endl;
            }
            dataQueue->clear();
            locker->unlock();
        }
    }
}

void connection(Client* client, string ip, int portListen, int port)
{
    system("cls");
    cout << "Connection ..." << endl;
    mutex locker;
    vector<string> dataQueue;
    dataQueue.clear();
    bool stopListening = false;

    thread listener = client->listenerSpawnThread(portListen, "tcp", &locker, &stopListening, &dataQueue);
    thread display(affichage, &locker, ip, &stopListening, &dataQueue);

    int target = client->connection(ip, port, "tcp");
    if(target < 0)
    {
        system("cls");
        if(target == -1) cout << "CONNECTION TIMED OUT" << endl;
        else cout << "CAN'T CONNECT TO THE ADRESSE - UNHANDLED ERROR" << endl;
        this_thread::sleep_for(chrono::seconds(2));
        stopListening = true;
        listener.join();
        display.join();
        return;
    }

    system("cls");
    cout << "Connecter a " << ip << endl << endl;

    string message;
    do
    {
        getline(cin, message);
        if(message != "//close") client->envoyerText(target, message);
        else
        {
            client->terminerConnection(target);
            stopListening = true;
        }
    } while(!stopListening);
    
    cout << "Closing chat ..." << endl;
    listener.join();
    display.join();
}

int main()
{
    Client client(5, 1024, 2048);
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
        cout << "1: changer IP" << endl;
        cout << "2: changer port" << endl;
        cout << "3: changer port d'ecoute" << endl;
        cout << "4: se connecter" << endl;
        cout << "5: quitter" << endl;

        cin >> selection;

        switch (selection)
        {
        case 1:
            system("cls");
            cout << "Nouvelle adresse : ";
            cin >> conf.addr;
            break;
        case 2:
            system("cls");
            cout << "Nouveau port : ";
            cin >> conf.portTalk;
            break;
        case 3:
            system("cls");
            cout << "Nouveau port : ";
            cin >> conf.portListen;
            break;
        case 4:
            connection(&client, conf.addr, conf.portListen, conf.portTalk);
            break;
        case 5:
            system("cls");
            exit = true;
            break;
        
        default:
            break;
        }
    } while(!exit);

    return 0;
}