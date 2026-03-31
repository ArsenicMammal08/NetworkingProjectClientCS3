#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#define PORT 2000
#define EXIT -1
#define MESSAGE 1
#define SNYC 0
#define MessageBufferSize 1024
#define IP "127.0.0.1"

struct ClientThreadArgs {
    int SocketFD;
    struct messageNode** phead;
    struct messageNode** ptail;
    int* messageCount;
    pthread_mutex_t* editMessagesList;
};
struct messageNode {
    char userName[32];
    char Message[1024];
    struct messageNode* next;
};
struct messageStruct {
    char userName[32];
    char Message[1024];
};
struct request {
    char userName[32];
    int RequestType;
    char Message[1024];
};
void sendmessage(int socketFD) {
    struct request request;
    request.RequestType = MESSAGE;
    request.Message[0] = 'h';
    request.Message[1] = 'e';
    request.Message[2] = 'l';
    request.Message[3] = 'l';
    request.Message[4] = 'o';
    request.Message[5] = ' ';
    request.Message[6] = 'w';
    request.Message[7] = 'o';
    request.Message[8] = 'r';
    request.Message[9] = 'l';
    request.Message[10] = 'd';
    request.Message[11] = '\0';
    request.userName[0] = 'd';
    request.userName[1] = 'e';
    request.userName[2] = 'r';
    request.userName[3] = 'e';
    request.userName[4] = 'k';
    request.userName[5] = '\0';
    send(socketFD, &request, sizeof(request),  0);
}
void disconnect(int socketFD) {
    struct request request;
    request.RequestType = EXIT;
    send(socketFD, &request, sizeof(request), 0);
}
struct messageStruct* getsyncdata(int socketFD, int* datasize) {
    struct request request;
    request.RequestType = SNYC;
    send(socketFD, &request, sizeof(request), 0);
    recv(socketFD, datasize, sizeof(int), 0);
    if (*datasize == 0) {
        return NULL;
    }
    struct messageStruct* messages = (struct messageStruct*)malloc(sizeof(struct messageStruct) * *datasize);

    recv(socketFD, messages, sizeof(struct messageStruct) * *datasize, 0);
    return messages;
}
struct messageNode* CreateMessageNode(struct messageStruct* input) {
    struct messageNode* node = (struct messageNode*)malloc(sizeof(struct messageNode));
    node->next = NULL;
    strcpy(node->userName, input->userName);
    strcpy(node->Message, input->Message);
    return node;
}
void syncdata(int socketFD, struct messageNode** ptail, struct messageNode** phead) {
    int datasize = 0;
    int startpos = 0;
    struct messageStruct* messages = getsyncdata(socketFD, &datasize);
    if (datasize == 0) {
        return;
    }
    if (!*ptail) {
        *ptail = CreateMessageNode(&(messages[0]));
        *phead = *ptail;
    }
    struct messageNode* pnode = *ptail;
    for (int i = startpos; i < datasize - 1; i++) {
        pnode->next = CreateMessageNode(&(messages[i]));
        pnode = pnode->next;
    }
    free(messages);
}
void printlist(struct messageNode** phead) {
    struct messageNode* pnode = *phead;
    while (pnode != NULL) {
        printf("userName: %s Message: %s\n", pnode->userName, pnode->Message);
        pnode = pnode->next;
    }
}
void window(int socketFD, struct messageNode** ptail, struct messageNode** phead) {
    InitWindow(800, 1000, "TextBox example");

    char text[64] = "";
    bool editMode = false;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Click to activate, click away to deactivate
        if (GuiTextBox((Rectangle){ 100, 100, 200, 30 }, text, 64, editMode))
            editMode = !editMode;


        EndDrawing();
    }

    CloseWindow();

}
int main(void) {
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in ServerAddr;
    bzero(&ServerAddr, sizeof(ServerAddr));
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = inet_addr(IP);
    ServerAddr.sin_port = htons(PORT);


    struct messageNode* phead = NULL;
    struct messageNode* ptail = NULL;
    /*
    while (connect(socketFD, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr)));

    printlist(&phead);
    disconnect(socketFD);
    */
    window(socketFD, &phead, &ptail);


}