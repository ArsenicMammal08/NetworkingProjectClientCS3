#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <arpa/inet.h>
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
    printf("\n userName: ");
    scanf("%s", &(request.userName));
    printf("\n Message: ");
    scanf("%s", &(request.Message));
    send(socketFD, &request, sizeof(request),  0);
}
void disconnect(int socketFD) {
    struct request request;
    request.RequestType = EXIT;
    send(socketFD, &request, sizeof(request), 0);
}
struct messageStruct* getsyncdata(int socketFD) {
    struct request request;
    request.RequestType = SNYC;
    int datasize;
    send(socketFD, &request, sizeof(request), 0);
    recv(socketFD, &datasize, sizeof(int), 0);
    struct messageStruct* messages = (struct messageStruct*)malloc(sizeof(struct messageStruct) * datasize);
    recv(socketFD, messages, sizeof(struct messageStruct) * datasize, 0);
    return messages;
}
void syncdata(int socketFD, struct messageNode** ptail) {

}
int main(void) {
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in ServerAddr;
    bzero(&ServerAddr, sizeof(ServerAddr));
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = inet_addr(IP);
    ServerAddr.sin_port = htons(PORT);
    connect(socketFD, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
    struct messageNode* phead = NULL;
    struct messageNode* ptail = NULL;



    disconnect(socketFD);


}