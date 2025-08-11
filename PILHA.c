#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <windows.h>

// SUGESTÃO 1: Adicionar o #pragma para facilitar a compilação.
#pragma comment(lib, "ws2_32.lib")

// Definição da estrutura Node e da Pilha (seu código original, está perfeito)
typedef struct Node {
    int id;
    struct Node* next;
} Node;

Node* head = NULL;
long long count = 0;

void push(int id) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (!newNode) { perror("malloc"); exit(EXIT_FAILURE); }
    newNode->id = id;
    newNode->next = head;
    head = newNode;
    count++;
}

int pop() {
    if (head == NULL) return -1;
    Node* temp = head;
    int id = temp->id;
    head = head->next;
    free(temp);
    count--;
    return id;
}

void shuffle(int *array, long long n) {
    if (n > 1) {
    long long i;
        for (i = 0; i < n - 1; i++) {
            long long j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

int main(int argc, char *argv[]) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Falha ao inicializar Winsock. Codigo de Erro: %d\n", WSAGetLastError());
        return 1;
    }

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <numero_de_IDs>\n", argv[0]);
        WSACleanup();
        return 1;
    }

    long long num_ids = atoll(argv[1]);

    srand((unsigned int)time(NULL));
    printf("Estrutura: Pilha (Stack) [Windows Version]\n");
    printf("Gerando e embaralhando %lld IDs...\n", num_ids);

    int* id_pool = (int*)malloc(sizeof(int) * num_ids);
    if (!id_pool) {
        perror("malloc");
        WSACleanup();
        return 1;
    }
    long long i;
    for (i = 0; i < num_ids; i++) { id_pool[i] = i + 1; }
    shuffle(id_pool, num_ids);
    for (i = 0; i < num_ids; i++) { push(id_pool[i]); }
    free(id_pool);

    printf("Pronto! %lld IDs carregados.\n\n", count);

    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("socket() falhou: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("bind() falhou: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, 10) == SOCKET_ERROR) {
        printf("listen() falhou: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("Servidor aguardando conexoes na porta 8080...\n");

    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) != INVALID_SOCKET) {
        char buffer[1024] = {0};
        char response[64];
        int recv_size = recv(new_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (recv_size == SOCKET_ERROR) {
            printf("recv() falhou: %d\n", WSAGetLastError());
            closesocket(new_socket);
            continue;
        }
        buffer[recv_size] = '\0';

        if (strncmp(buffer, "GET_ID", 6) == 0) {
            int new_id = pop();
            if (new_id != -1) {
                sprintf(response, "ID:%d", new_id);
            } else {
                sprintf(response, "END");
            }
            // SUGESTÃO 2: Adicionar uma verificação opcional para o send()
            if (send(new_socket, response, (int)strlen(response), 0) == SOCKET_ERROR) {
                printf("send() falhou: %d\n", WSAGetLastError());
            }
        }
        closesocket(new_socket);
    }

    closesocket(server_fd);
    WSACleanup();

    return 0;
}
