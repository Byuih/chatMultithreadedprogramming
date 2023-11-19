#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

std::mutex mtx;
std::vector<std::string> chatLog;

void logMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);
    chatLog.push_back(message);
}

void printChatLog() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto& message : chatLog) {
            std::cout << message << std::endl;
        }
        chatLog.clear();
    }
}

void handleClient(SOCKET clientSocket) {
    char buffer[4096];
    std::string message;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        }

        message = buffer;
        logMessage("Client: " + message);
        if (message == "exit")
            break;

        memset(buffer, 0, sizeof(buffer));
        std::cout << "Enter your message: ";
        std::cin.getline(buffer, sizeof(buffer));
        std::string reply(buffer);

        logMessage("Server: " + reply);
        send(clientSocket, reply.c_str(), reply.size(), 0);
    }

    closesocket(clientSocket);
}

int main() {
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);
    int wsOK = WSAStartup(ver, &wsData);
    if (wsOK != 0) {
        std::cerr << "Can't initialize winsock!" << std::endl;
        return -1;
    }

    SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == INVALID_SOCKET) {
        std::cerr << "Can't create a socket!" << std::endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
    hint.sin_addr.S_un.S_addr = INADDR_ANY;

    bind(listening, (sockaddr*)&hint, sizeof(hint));
    listen(listening, SOMAXCONN);

    std::cout << "Waiting for client connection..." << std::endl;

    sockaddr_in client;
    int clientSize = sizeof(client);
    SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    ZeroMemory(host, NI_MAXHOST);
    ZeroMemory(service, NI_MAXSERV);

    if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
        std::cout << host << " connected on port " << service << std::endl;
    }
    else {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;
    }

    closesocket(listening);

    std::thread logThread(printChatLog);
    std::thread clientThread(handleClient, clientSocket);

    clientThread.join();
    logThread.join();

    WSACleanup();

    return 0;
}