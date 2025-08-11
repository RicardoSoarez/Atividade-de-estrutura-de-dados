#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

typedef struct Node {
    int id;
    struct Node* next;
} Node;

Node* head = NULL;
Node* tail = NULL;
long long count = 0;

void enqueue(int id) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) {
        fprintf(stderr, "Falha ao alocar memoria\n");
        exit(1);
    }
    
    new_node->id = id;
    new_node->next = NULL;

    if (!tail) {
        head = tail = new_node;
    } else {
        tail->next = new_node;
        tail = new_node;
    }
    count++;
}

int dequeue() {
    if (!head) return -1;

    Node* temp = head;
    int id = temp->id;
    head = head->next;

    if (!head) tail = NULL;
    
    free(temp);
    count--;
    return id;
}

void shuffle(int* array, long long n) {
    if (n <= 1) return;
    long long i;
    for (i = n - 1; i > 0; i--) {
        long long j = rand() % (i + 1);
        int temp = array[j];
        array[j] = array[i];
        array[i] = temp;
    }
}

SOCKET init_server(int port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "Falha no WSAStartup\n");
        return INVALID_SOCKET;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "Falha ao criar socket\n");
        return INVALID_SOCKET;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        fprintf(stderr, "Falha no bind\n");
        closesocket(sock);
        return INVALID_SOCKET;
    }

    listen(sock, SOMAXCONN);
    return sock;
}

void clean_up(SOCKET sock) {
    while (dequeue() != -1);
    closesocket(sock);
    WSACleanup();
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <quantidade_ids>\n", argv[0]);
        return 1;
    }

    long long n = atoll(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Quantidade deve ser positiva\n");
        return 1;
    }

    srand((unsigned)time(NULL));
    printf("Inicializando fila com %lld IDs...\n", n);

    int* ids = (int*)malloc(n * sizeof(int));
    if (!ids) {
        fprintf(stderr, "Falha ao alocar IDs\n");
        return 1;
    }
    long long i;
    for (i = 0; i < n; i++) {
        ids[i] = (int)(i + 1);
    }
    shuffle(ids, n);
    
    for (i = 0; i < n; i++) {
        enqueue(ids[i]);
    }
    free(ids);

    SOCKET server = init_server(8080);
    if (server == INVALID_SOCKET) {
        clean_up(server);
        return 1;
    }

    printf("Servidor pronto na porta 8080\n");

    while (1) {
        struct sockaddr_in client;
        int addrlen = sizeof(client);
        SOCKET client_sock = accept(server, (struct sockaddr*)&client, &addrlen);

        if (client_sock == INVALID_SOCKET) {
            fprintf(stderr, "Falha no accept\n");
            continue;
        }

        char buf[32];
        int received = recv(client_sock, buf, sizeof(buf), 0);

        if (received > 0) {
            buf[received] = '\0';
            if (strcmp(buf, "GET_ID") == 0) {
                int id = dequeue();
                char response[16];
                if (id != -1) {
                    sprintf(response, "ID:%d", id);
                } else {
                    strcpy(response, "END");
                }
                send(client_sock, response, strlen(response), 0);
            }
        }

        closesocket(client_sock);
    }

    clean_up(server);
    return 0;
}
