#include <iostream>
#include <vector>
#include <string>
#include <thread>

#include "WinSock2.h"
#include "windows.h"
#include "client.hpp"

using namespace std;

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
    bool exit = false;
    int port = 55000;
    int portListen = 55000;
    string ip = "127.0.0.1";
    int selection;

    do
    {
        system("cls");
        cout << "DESTINATION :" << endl;
        cout << "Adresse : " << ip << endl;
        cout << "Port : " << port << endl << endl;
        cout << "HOTE :" << endl;
        cout << "Port d'ecoute : " << portListen << endl;
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
            cin >> ip;
            break;
        case 2:
            system("cls");
            cout << "Nouveau port : ";
            cin >> port;
            break;
        case 3:
            system("cls");
            cout << "Nouveau port : ";
            cin >> portListen;
            break;
        case 4:
            connection(&client, ip, portListen, port);
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