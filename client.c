#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

#define PORT 8080
#define BUFFER_LEN 200

char board[3][3];
char myPlayer = 'X';
char opponentPlayer = 'O';
SOCKET ConnectSocket;
HANDLE recvThread;
volatile bool myTurn = true; // X börjar alltid

void ritaBord() {
    printf("\n");
    for (int i = 0; i < 3; i++) {
        printf(" %c | %c | %c \n", board[i][0], board[i][1], board[i][2]);
        if (i < 2) printf("---|---|---\n");
    }
    printf("\n");
}

bool checkWin(char player) {
    for (int i = 0; i < 3; i++) {
        if (board[i][0]==player && board[i][1]==player && board[i][2]==player) return true;
        if (board[0][i]==player && board[1][i]==player && board[2][i]==player) return true;
    }
    if (board[0][0]==player && board[1][1]==player && board[2][2]==player) return true;
    if (board[0][2]==player && board[1][1]==player && board[2][0]==player) return true;
    return false;
}

bool boardFull() {
    for (int i=0;i<3;i++)
        for (int j=0;j<3;j++)
            if (board[i][j]==' ') return false;
    return true;
}

// Tråd som tar emot drag från motståndaren
DWORD WINAPI receiveThread(LPVOID lpParam) {
    char buffer[BUFFER_LEN];
    char leftover[BUFFER_LEN] = "";
    
    while (1) {
        int bytes = recv(ConnectSocket, buffer, BUFFER_LEN-1, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';

        char temp[BUFFER_LEN*2];
        snprintf(temp, sizeof(temp), "%s%s", leftover, buffer);

        char *line = strtok(temp, "\n");
        while (line != NULL) {
            int row, col;
            if (sscanf(line, "DRAG %d %d", &row, &col) == 2) {
                board[row-1][col-1] = opponentPlayer;
                printf("Motståndaren spelar: %d %d\n", row, col);
                ritaBord();

                if (checkWin(opponentPlayer)) {
                    printf("Du förlorade!\n");
                    exit(0);
                }
                if (boardFull()) {
                    printf("Oavgjort!\n");
                    exit(0);
                }

                myTurn = true; // Nu är det min tur
            }

            line = strtok(NULL, "\n");
        }

        // Spara kvarvarande data utan newline
        char *lastNewline = strrchr(temp, '\n');
        if (lastNewline) strcpy(leftover, lastNewline+1);
        else leftover[0] = '\0';
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("Socket error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(ConnectSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Connect error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Initiera bräde
    for (int i=0;i<3;i++)
        for (int j=0;j<3;j++)
            board[i][j]=' ';

    // Matchningskod
    char code[50];
    printf("Ange matchningskod: ");
    scanf("%s", code);
    send(ConnectSocket, code, (int)strlen(code), 0);

    char buffer[BUFFER_LEN];
    int bytes = recv(ConnectSocket, buffer, BUFFER_LEN-1, 0);
    buffer[bytes] = '\0';

    if (strcmp(buffer, "MATCH_FOUND\n") == 0) {
        printf("Matchning hittad! Spelet startar...\n");
    } else {
        printf("Väntar på motståndare...\n");
        while (1) {
            bytes = recv(ConnectSocket, buffer, BUFFER_LEN-1, 0);
            if (bytes <= 0) {
                printf("Server stängde anslutningen.\n");
                return 0;
            }
            buffer[bytes] = '\0';
            if (strcmp(buffer, "MATCH_FOUND\n") == 0) break;
        }
        printf("Matchning hittad! Spelet startar...\n");
    }

    // Starta tråd för att ta emot drag
    recvThread = CreateThread(NULL, 0, receiveThread, NULL, 0, NULL);

    // Huvud-loop
    ritaBord(); // Rita initialt bräde
    while (1) {
        if (myTurn) {
            int row, col;
            printf("Din tur (%c). Ange rad (1-3) och kolumn (1-3): ", myPlayer);
            while (1) {
                scanf("%d %d", &row, &col);
                if (row>=1 && row<=3 && col>=1 && col<=3 && board[row-1][col-1]==' ') break;
                printf("Ogiltigt drag, försök igen: ");
            }

            board[row-1][col-1] = myPlayer;
            ritaBord();

            sprintf(buffer, "DRAG %d %d\n", row, col); // Lägg till newline
            send(ConnectSocket, buffer, (int)strlen(buffer), 0);

            if (checkWin(myPlayer)) {
                printf("Du vann!\n");
                break;
            }
            if (boardFull()) {
                printf("Oavgjort!\n");
                break;
            }

            myTurn = false;
        } else {
            static bool printed = false;
            if (!printed) {
                printf("Väntar på motståndarens drag...\n");
                printed = true;
            }
            Sleep(500);
        }
    }

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}
