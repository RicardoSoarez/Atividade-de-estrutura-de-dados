#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 64
#define MAX_ATTEMPTS 3

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <tempo_em_segundos>\n", argv[0]);
        return 1;
    }

    const int duration = atoi(argv[1]);
    if (duration <= 0) {
        fprintf(stderr, "Tempo deve ser positivo\n");
        return 1;
    }

    // Inicializa Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup falhou: %d\n", WSAGetLastError());
        return 1;
    }

    long long ids_received = 0;
    const time_t start_time = time(NULL);
    printf("Benchmark iniciado por %d segundos...\n", duration);

    while (difftime(time(NULL), start_time) < duration) {
        SOCKET sock = INVALID_SOCKET;
        struct sockaddr_in server;
        int attempts = 0;

        // Tenta conectar (com retry)
        while (attempts < MAX_ATTEMPTS) {
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET) {
                attempts++;
                continue;
            }

            server.sin_family = AF_INET;
            server.sin_addr.s_addr = inet_addr(SERVER_IP);
            server.sin_port = htons(SERVER_PORT);

            if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == 0) {
                break; // Conex?o bem-sucedida
            }

            closesocket(sock);
            sock = INVALID_SOCKET;
            attempts++;
        }

        if (sock == INVALID_SOCKET) {
            continue; // Falha ap?s v?rias tentativas
        }

        // Envia requisi??o
        const char *request = "GET_ID";
        if (send(sock, request, strlen(request), 0) == SOCKET_ERROR) {
            closesocket(sock);
            continue;
        }

        // Recebe resposta
        char response[BUFFER_SIZE] = {0};
        int bytes_received = recv(sock, response, BUFFER_SIZE - 1, 0);
        closesocket(sock);

        if (bytes_received > 0) {
            response[bytes_received] = '\0';
            if (strncmp(response, "END", 3) == 0) {
                printf("Servidor sem IDs disponiveis\n");
                break;
            }
            ids_received++;
        }
    }

    // Resultados
    const double elapsed_time = difftime(time(NULL), start_time);
    printf("\n=== Resultados ===\n");
    printf("Tempo total: %.2f segundos\n", elapsed_time);
    printf("IDs recebidos: %lld\n", ids_received);
    
    if (elapsed_time > 0) {
        printf("Taxa: %.2f IDs/segundo\n", ids_received / elapsed_time);
    }

    WSACleanup();
    return 0;
}
