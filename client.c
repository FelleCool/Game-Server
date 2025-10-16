#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define PORT 8080
#define BUFFER_LENGTH 200

// Rita brädet
void ritaBord(char board[3][3]) {
    printf("\n");
    for (int i = 0; i < 3; i++) {
        printf(" %c | %c | %c \n", board[i][0], board[i][1], board[i][2]);
        if (i < 2) printf("---|---|---\n");
    }
    printf("\n");
}

// Kontrollera om någon har vunnit
char checkWinner(char board[3][3]) {
    for (int i = 0; i < 3; i++) {
        // Rad
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2])
            return board[i][0];
        // Kolumn
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i])
            return board[0][i];
    }
    // Diagonaler
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2])
        return board[0][0];
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0])
        return board[0][2];
    return ' ';
}

// Kontrollera om brädet är fullt
int isFull(char board[3][3]) {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (board[i][j] == ' ') return 0;
    return 1;
}

int main() {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_LENGTH];
    int result;

    // Initiera Winsock
    result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }

    // Skapa socket
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Serveradress
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // localhost

    // Anslut till servern
    result = connect(ConnectSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        printf("Unable to connect to server! %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Ansluten till servern!\n");

    // Initiera bräde
    char board[3][3] = {
        {' ', ' ', ' '},
        {' ', ' ', ' '},
        {' ', ' ', ' '}
    };

    char currentPlayer = 'X';
    int row, col;

    while (1) {
        ritaBord(board);
        printf("Spelare %c, ange rad (1-3) och kolumn (1-3): ", currentPlayer);
        scanf("%d %d", &row, &col);

        if (row < 1 || row > 3 || col < 1 || col > 3 || board[row-1][col-1] != ' ') {
            printf("Ogiltigt drag. Försök igen.\n");
            continue;
        }

        board[row-1][col-1] = currentPlayer;

        // Skicka draget till servern
        sprintf(buffer, "%d %d %c", row, col, currentPlayer);
        send(ConnectSocket, buffer, strlen(buffer), 0);

        // Läs svar
        int bytesReceived = recv(ConnectSocket, buffer, BUFFER_LENGTH, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            printf("Server: %s\n", buffer);
        }

        // Kontrollera vinnare
        char winner = checkWinner(board);
        if (winner != ' ') {
            ritaBord(board);
            printf("Spelare %c vinner!\n", winner);
            break;
        }

        if (isFull(board)) {
            ritaBord(board);
            printf("Oavgjort!\n");
            break;
        }

        // Växla spelare
        currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    }

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}