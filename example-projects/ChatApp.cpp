#include "ws2cpp.h"
#include <iostream>
#include <string>

void write(const char*);
void writeLine(const char*);

using namespace std;

#define PORT 32719
#define MAXMSGSIZE 999

int main()
{
    SocketAddress addresses[16];
    int curLen = 0;

    SockUtils::Initialize();
    SockUtils::PrintWSAD();

    int selection = 0;

    cout << "Welcome to chat app!" << endl;


    SocketMessage msg(0, "err", 4);
    UDPSocket* sock = SockUtils::CreateUDPSocket();
    if (sock == nullptr) {
        cout << "Failed to create socket! Terminating..." << endl;
        SockUtils::PrintLastError();
        return -1;
    }
    AddressInfo* infs[8];
    int infCount = 0;
    AddressInfo* inf = SockUtils::GetClientAddresses();
    SocketAddress* sockAddr = SockUtils::GetClientAddresses()->GetSocketAddress();
    while (inf != nullptr) {
        infs[infCount++] = inf;
        inf = inf->GetNext();
    }
    int hostSel = 0;
    cout << "Select your IP address to connect/host" << endl;
    for (int i = 0; i < infCount; i++) {
        cout << (i + 1) << "- " << infs[i]->GetSocketAddress()->GetIP() << endl;
    }
    cout << "Selection: ";
    cin >> hostSel;
    if (hostSel < infCount + 1 && hostSel > 0) {
        sockAddr = infs[hostSel - 1]->GetSocketAddress();
    }
    else {
        cout << "Invalid selection! Terminating..." << endl;
        return -1;
    }
    int bindres = sock->Bind(*sockAddr, PORT, true);
    if (bindres != 0) {
        cout << "Failed to bind socket! Terminating..." << endl;
        SockUtils::PrintLastError();
        return -1;
    }
    cout << "Binded socket at IP: " << sockAddr->GetIP() << " and port: " << sockAddr->GetAddrPort() << endl;
    while (true) {
        cout << "What would you like to do?" << endl;
        cout << "1 - Act as a host\n2 - Send messages\n3 - Read messages" << endl << "Selection: ";
        cin >> selection;
        if (selection == 1) {
            cout << "Hosting server at IP: " << sockAddr->GetIP() << " and port: " << sockAddr->GetAddrPort() << endl;
            while (true) {
                SocketAddress senderAddr;
                int result = sock->Receive(&msg, &senderAddr);
                if (result > 0) {
                    if (msg.code == 0) {
                        cout << msg.message << endl;
                        for (int i = 0; i < curLen; i++) {
                            SocketAddress curAddr = addresses[i];
                            cout << "Forwarding message to IP: " << curAddr.GetIP() << " at port: " << curAddr.GetAddrPort() << endl;
                            sock->Send(msg, curAddr);
                        }
                    }
                    else if (msg.code == 1) {
                        if (curLen < 16) {
                            addresses[curLen++] = senderAddr;
                            SocketMessage ackMsg(2, "accepted", 9);
                            sock->Send(ackMsg, senderAddr);
                            cout << "New reader joined..." << endl;
                            cout << "Sending ACK message back to IP: " << senderAddr.GetIP() << " at port: " << senderAddr.GetAddrPort() << endl;
                        }
                        else {
                            cout << "New reader failed to connect. Room full!" << endl;
                        }
                    }
                }
                else if (result == -1) {
                    SockUtils::PrintLastError();
                    cout << "Error while receiving! Terminating..." << endl;
                    return 1;
                }
            }
        }
        else if (selection == 2) {
            string username = "u-";
            string message = "";
            string fullmsg = "";
            string ip; int port;
            cout << "Enter '-back' to go back." << endl;
            cout << "Enter the IP address of the target host: ";
            cin >> ip;
            cout << "Enter the port of the target host: ";
            cin >> port;
            cout << "Enter your username: ";
            getline(cin, username);
            getline(cin, username);
            SocketAddress addr(ip.c_str(), port);

            while (true) {
                cout << "Enter your message: ";
                getline(cin, message);
                int len = message.length();
                if (message == "") {
                    cout << endl;
                    continue;
                } else if (message == "-back") {
                    break;
                } else if(message.length() > MAXMSGSIZE) {
                    cout << "Message too big! (Max size: " << MAXMSGSIZE << ", yours: " << len << ")" << endl;
                    continue;
                }
                fullmsg = username + " >> " + message;
                msg.code = 0;
                msg.message = const_cast<char*>(fullmsg.c_str());
                msg.length = fullmsg.length() + 1;
                sock->Send(msg, addr);
            }
        }
        else if (selection == 3) {
            string ip; int port; bool acked = false;
            cout << "Enter the IP address of the target host: ";
            cin >> ip;
            cout << "Enter the port of the target host: ";
            cin >> port;
            SocketAddress addr(ip.c_str(), port);
            cout << "Sending message to IP: " << addr.GetIP() << " at port: " << addr.GetAddrPort() << endl;
            SocketMessage msgForAck(1, reinterpret_cast<char*>(sockAddr), sizeof(SocketAddress));
            sock->Send(msgForAck, addr);
            while (true) {
                int result = sock->Receive(&msg, nullptr);
                if (result > 0) {
                    if (!acked && msg.code == 2) {
                        acked = true;
                        cout << "Connected to host" << endl;
                    }
                    else if (acked && msg.code == 0) {
                        cout << msg.message << endl;
                    }
                }
                else if (result == -1) {
                    SockUtils::PrintLastError();
                    cout << "Error while receiving! Terminating..." << endl;
                    return 1;
                }
            }
        }
    }
    
    SockUtils::Dispose();
}

void write(const char* text) {
    std::cout << text;
}

void writeLine(const char* text) {
    std::cout << text << std::endl;
}