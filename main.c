#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>


#define RECV_BUF_SIZE 512
#define SEND_BUF_SIZE 512
#define N_WORKERS 5


typedef struct Node Node;
typedef struct Queue Queue;

pthread_mutex_t queue_mtx;


struct Node {
    int value;
    Node *next;
};


struct Queue {
    Node *head;
    Node *tail;
};


static Queue clients_queue;


Node *create_node(int client_socket) {
    Node *node = calloc(1, sizeof(Node));
    node->value = client_socket;
    node->next = NULL;
    return node;
}


void insert_to_queue(Queue *target_queue, int value) {
    pthread_mutex_lock(&queue_mtx);

    Node *cur_node = create_node(value);

    if (!target_queue->head) {
        target_queue->head = cur_node;
        target_queue->tail = cur_node;
        pthread_mutex_unlock(&queue_mtx);
        return;
    }

    target_queue->tail->next = cur_node;
    target_queue->tail = cur_node;
    pthread_mutex_unlock(&queue_mtx);
}


int pop_from_queue(Queue *target_queue) {
    // Если очередь пустая возвращаем -1
    pthread_mutex_lock(&queue_mtx);
    if (!target_queue->head) {
        pthread_mutex_unlock(&queue_mtx);
        return -1;
    }

    Node *cur_node = target_queue->head;
    int value = cur_node->value;

    // Если это не последний элемент
    if (cur_node->next) {
        target_queue->head = cur_node->next;
    } else {
        target_queue->head = NULL;
        target_queue->tail = NULL;
    }

    free(cur_node);
    pthread_mutex_unlock(&queue_mtx);

    return value;
}


void tcp_server() {
    int rc;

    // create the server socket
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // define the server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9004);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to our specified IP and port
    rc = bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));

    if (rc < 0) {
        perror("HH_ERROR: bind() call failed");
        exit(1);
    }

    // second agrument is a backlog - how many connections can be waiting for this socket simultaneously
    rc = listen(server_socket, 10);

    if (rc < 0) {
        perror("HH_ERROR: listen() call failed");
        exit(1);
    }

    printf("TCP server started.\n");

    while (1) {
        int client_socket;
        client_socket = accept(server_socket, NULL, NULL);
        insert_to_queue(&clients_queue, client_socket);
    }
    // close the socket
    close(server_socket);
}


void processing_client_service(int client_socket) {
    // получить client_socket
    // отправить в него данные

    int rcvd, metric_id;
    char *buf = malloc(RECV_BUF_SIZE);

    rcvd = recv(client_socket, buf, RECV_BUF_SIZE, 0);

    if (rcvd < 0)    // receive error
        printf(("recv() error\n"));
    else if (rcvd == 0)    // receive socket closed
        printf("Client disconnected upexpectedly.\n");
    else { // message received

        printf("Received query. Client socket %d.\n", client_socket);

        sscanf(buf, "%d", &metric_id);

        switch (metric_id) {
            case 1:
                // top_bio_words
                printf("%d\n", metric_id);
        }

    }

    send(client_socket, buf, RECV_BUF_SIZE, 0);
    free(buf);
    close(client_socket);
}


void *tcp_server_worker(void *arg) {
    int client_socket = -1;
    while (1) {
        client_socket = pop_from_queue(&clients_queue);
        while (client_socket == -1) {
            usleep(1000);
            client_socket = pop_from_queue(&clients_queue);
        }
        processing_client_service(client_socket);
    }
}


int main() {
    pthread_t threads[N_WORKERS];
    for (int i = 0; i < N_WORKERS; i++) {
        int result = pthread_create(&threads[i], NULL, tcp_server_worker, NULL);
        if (result) {
            printf("Thread could not be created. Error number: %d. Thread number %d\n", result, i);
            exit(1);
        } else {
            printf("Worker thread created. Worker number %d\n", i);
        }

    }

    tcp_server();
    return 0;
}
