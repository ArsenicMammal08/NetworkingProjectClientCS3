#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <SDL2/SDL.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"

#define PORT             2000
#define EXIT        -1
#define MESSAGE          1
#define SNYC             0
#define IP               "127.0.0.1"
#define WINDOW_W 700
#define WINDOW_H 600
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
void sendmessage(int socketFD, char* userName, char* Message){
    struct request request;
    request.RequestType = MESSAGE;
    strncpy(request.userName, userName, sizeof(request.userName) - 1);
    strncpy(request.Message,  Message,  sizeof(request.Message) - 1);
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
    SDL_Init(SDL_INIT_VIDEO);

    // 2. Create the window
    SDL_Window* win = SDL_CreateWindow(
        "My Window",              // title
        SDL_WINDOWPOS_CENTERED,   // x position
        SDL_WINDOWPOS_CENTERED,   // y position
        700, 600,                 // width, height
        SDL_WINDOW_SHOWN          // flags
    );

    // 3. Create the renderer (does the actual drawing)
    SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    // 4. Init Nuklear on top of SDL
    struct nk_context* ctx = nk_sdl_init(win, renderer);

    // 5. Main loop
    int running = 1;
    SDL_Event evt;
    while (running) {
        // handle input
        nk_input_begin(ctx);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) running = 0;
            nk_sdl_handle_event(&evt);
        }
        nk_input_end(ctx);

        // draw your UI here with nk_begin / nk_end etc.

        // render
        SDL_RenderClear(renderer);
        nk_sdl_render(NK_ANTI_ALIASING_ON);
        SDL_RenderPresent(renderer);
    }

    // 6. Cleanup
    nk_sdl_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
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