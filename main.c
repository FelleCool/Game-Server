#include <stdio.h>
#include <stdbool.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

int main() {
    printf("start\n");
    WSADATA wsaData;
    int iResult;

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
    printf("Error at socket(): %d\n", WSAGetLastError());
    WSACleanup();
    return 1;
    }

    struct sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;     
    service.sin_port = htons(8080);            


    int opt = 1;
    
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));


    
    iResult = bind(listenSocket, (SOCKADDR*)&service, sizeof(service));
    if (iResult == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }


    //lisener
    bool lisenig = true;
    while(lisenig == true){
        listen(listenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
        }
        printf("Waiting for a client.\n");

        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
        }
        printf("Client connected!\n");
    }

    closesocket(listenSocket);
    WSACleanup();  
    return 0;
}