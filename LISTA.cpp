#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024
#define RESPONSE_SIZE 64

// Estrutura do n? com otimiza??o de alinhamento
typedef struct __declspec(align(8)) Node {
    int id;
    struct Node* next;
} Node;

// Vari?veis globais com prote??o b?sica
static Node* volatile head = NULL;
static Node* volatile tail = NULL;
static volatile long long count = 0;
static CRITICAL_SECTION cs;  // Se??o cr?tica para thread safety

// Inicializa a lista
void init_list() {
    InitializeCriticalSection(&cs);
}

// Destr?i a lista
void destroy_list() {
    EnterCriticalSection(&cs);
    Node* current = head;
    while (current != NULL) {
        Node* next = current->next;
        free(current);
        current = next;
    }
    head = tail = NULL;
    count = 0;
    LeaveCriticalSection(&cs);
    DeleteCriticalSection(&cs);
}

// Adiciona elemento no fim da lista (O(1))
void append_to_list(int id) {
    Node* newNode = (Node*)_aligned_malloc(sizeof(Node), 8);
    if (!newNode) {
        fprintf(stderr, "Falha na alocacao de memoria\n");
        exit(EXIT_FAILURE);
    }
    newNode->id = id;
    newNode->next = NULL;

    EnterCriticalSection(&cs);
    if (tail == NULL) {
        head = tail = newNode;
    } else {
        tail->next = newNode;
        tail = newNode;
    }
    count++;
    LeaveCriticalSection(&cs);
}

// Remove elemento do in?cio da lista (O(1))
int remove_from_head() {
    EnterCriticalSection(&cs);
    if (head == NULL) {
        LeaveCriticalSection(&cs);
        return -1;
    }

    Node* temp = head;
    int id = temp->id;
    head = head->next;

    if (head == NULL) {
        tail = NULL;
    }
    
    _aligned_free(temp);
    count--;
    LeaveCriticalSection(&cs);
    return id;
}

// Embaralha array usando Fisher-Yates (O(n))
void shuffle(int *array, long long n) {
    if (n <= 1) return;
    long long i;
    for (i = n - 1; i > 0; i--) {
        long long j = rand() % (i + 1);
        int temp = array[j];
        array[j] = array[i];
        array[i] = temp;
    }
}

// Inicializa o servidor TCP
SOCKET init_server() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup falhou: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket() falhou: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    // Configura??es de performance
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind() falhou: %d\n", WSAGetLastError());
        closesocket(sock);
        return INVALID_SOCKET;
    }

    if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "listen() falhou: %d\n", WSAGetLastError());
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <numero_de_IDs>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const long long num_ids = atoll(argv[1]);
    if (num_ids <= 0) {
        fprintf(stderr, "Numero de IDs deve ser positivo\n");
        return EXIT_FAILURE;
    }

    srand((unsigned int)time(NULL));
    init_list();

    printf("Estrutura: Lista Encadeada Otimizada\n");
    printf("Gerando e embaralhando %lld IDs...\n", num_ids);

    // Aloca??o otimizada
    int* id_pool = (int*)_aligned_malloc(num_ids * sizeof(int), 16);
    if (!id_pool) {
        fprintf(stderr, "Falha ao alocar memoria para IDs\n");
        destroy_list();
        return EXIT_FAILURE;
    }

    // Gera??o paraleliz?vel (se adaptado para threads)
    long long i;
    for (i = 0; i < num_ids; i++) {
        id_pool[i] = (int)(i + 1);
    }
    shuffle(id_pool, num_ids);

    // Carregamento na lista
    for (i = 0; i < num_ids; i++) {
        append_to_list(id_pool[i]);
    }
    _aligned_free(id_pool);

    printf("Pronto! %lld IDs carregados.\n\n", count);

    SOCKET server = init_server();
    if (server == INVALID_SOCKET) {
        destroy_list();
        return EXIT_FAILURE;
    }

    printf("Servidor aguardando conexoes na porta %d...\n", PORT);

    while (1) {
        SOCKET client;
        struct sockaddr_in client_addr;
        int addrlen = sizeof(client_addr);

        client = accept(server, (struct sockaddr*)&client_addr, &addrlen);
        if (client == INVALID_SOCKET) {
            fprintf(stderr, "accept() falhou: %d\n", WSAGetLastError());
            continue;
        }

        char buffer[BUFFER_SIZE];
        int recv_size = recv(client, buffer, BUFFER_SIZE - 1, 0);
        if (recv_size <= 0) {
            closesocket(client);
            continue;
        }
        buffer[recv_size] = '\0';

        if (strncmp(buffer, "GET_ID", 6) == 0) {
            char response[RESPONSE_SIZE];
            int id = remove_from_head();
            if (id != -1) {
                _snprintf(response, RESPONSE_SIZE, "ID:%d", id);
            } else {
                strncpy(response, "END", RESPONSE_SIZE);
            }
            send(client, response, (int)strlen(response), 0);
        }
        closesocket(client);
    }

    closesocket(server);
    destroy_list();
    WSACleanup();
    return 0;
}
