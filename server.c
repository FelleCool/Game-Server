#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_LEN 200
#define MAX_PLAYERS 100

typedef struct {
    SOCKET socket;
    char code[50];
    bool paired;
} Player;

Player players[MAX_PLAYERS];
int playerCount = 0;
CRITICAL_SECTION cs;

void sendTo(SOCKET sock, const char* msg) {
    send(sock, msg, (int)strlen(msg), 0);
}

/*void handleMove(SOCKET* player, ){

}
*/
DWORD WINAPI handleClient(LPVOID lpParam) {
    SOCKET clientSocket = *(SOCKET*)lpParam;
    free(lpParam);

    char buffer[BUFFER_LEN];
    char code[50];
    bool matched = false;

    // Ta emot matchningskod
    int bytes = recv(clientSocket, buffer, BUFFER_LEN - 1, 0);
    if (bytes <= 0) {
        closesocket(clientSocket);
        return 0;
    }
    buffer[bytes] = '\0';
    strcpy(code, buffer);

    EnterCriticalSection(&cs);
    // Lägg till spelare i listan
    players[playerCount].socket = clientSocket;
    strcpy(players[playerCount].code, code);
    players[playerCount].paired = false;
    playerCount++;

    // Kolla om det finns någon annan med samma kod
    SOCKET opponentSocket = INVALID_SOCKET;
    for (int i = 0; i < playerCount - 1; i++) {
        if (!players[i].paired && strcmp(players[i].code, code) == 0) {
            opponentSocket = players[i].socket;
            players[i].paired = true;
            players[playerCount - 1].paired = true;
            break;
        }
    }
    LeaveCriticalSection(&cs);

    if (opponentSocket != INVALID_SOCKET) {
        // Informera båda spelare att matchning finns
        sendTo(clientSocket, "MATCH_FOUND\n");
        sendTo(opponentSocket, "MATCH_FOUND\n");
        matched = true;
    } else {
        sendTo(clientSocket, "WAITING\n");
    }

    // Vidarebefordra meddelanden mellan spelare
    while (true) {
        //@@@ conrolera om spelarena vill/har sluta 
        int bytesReceivedC = recv(clientSocket, buffer, BUFFER_LEN - 1, 0);
        printf("byteC: %d\n" ,bytesReceivedC);

        if(bytesReceivedC == SOCKET_ERROR){
           int error = WSAGetLastError();
            printf("error: %d\n", error);
        }
        
        if (bytesReceivedC > 0) {
            printf("client");
            buffer[bytesReceivedC] = '\0';
            // Skicka till opponent om matchad
            if (matched && opponentSocket != INVALID_SOCKET) {
                send(opponentSocket, buffer, bytesReceivedC, 0);
            }
        }
        


        int bytesReceivedO = recv(opponentSocket, buffer, BUFFER_LEN - 1, 0);
        printf("byteO: %d\n" ,bytesReceivedO);

        if(bytesReceivedO == SOCKET_ERROR){
           int error = WSAGetLastError();
            printf("error: %d\n", error);
        }

        if (bytesReceivedO > 0) {
            printf("oponent");
            buffer[bytesReceivedO] = '\0';
            // Skicka till opponent om matchad
            if (matched && clientSocket != INVALID_SOCKET) {
                send(clientSocket, buffer, bytesReceivedO, 0);
            }
        }
        
    }

    closesocket(clientSocket);
    return 0;
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    InitializeCriticalSection(&cs);

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    listen(listenSocket, SOMAXCONN);

    printf("Server listening on port 8080\n");

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            SOCKET* pSock = malloc(sizeof(SOCKET));
            *pSock = clientSocket;
            CreateThread(NULL, 0, handleClient, pSock, 0, NULL);
        }
    }

    WSACleanup();
    DeleteCriticalSection(&cs);
    return 0;
}
