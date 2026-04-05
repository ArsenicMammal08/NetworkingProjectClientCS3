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
#define MaxLine 256
#define NameSize 32
#define MessageSize 1024
#define MaxHistory 1024
#define PORT             2000
#define EXIT        -1
#define MESSAGE          1
#define SNYC             0
#define IP               "127.0.0.1"
#define WINDOW_W 800
#define WINDOW_H 1000
typedef enum {
    SCREEN_NAME,
    SCREEN_CHAT
} Screen;
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
typedef struct {
    Screen Screen;
    char line[MaxLine];
    int is_mine;
} ChatLine;
typedef struct {
    Screen Screen;
    char NameBuff[NameSize];
    int name_len;

    ChatLine history[MaxHistory];
    int history_count;
    char InputBuff[MessageSize];
    int input_len;
    int scroll_to_bottom;
} AppState;
struct SyncThreadStruct {
    struct messageNode** phead;
    struct messageNode** ptail;
    pthread_mutex_t* SocketMutex;
    int socketFD;
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
        startpos++;
    }
    struct messageNode* pnode = *ptail;
    for (int i = startpos; i < datasize; i++) {
        pnode->next = CreateMessageNode(&(messages[i]));
        pnode = pnode->next;
    }
    *ptail = pnode;
    free(messages);
}
void printlist(struct messageNode** phead) {
    struct messageNode* pnode = *phead;
    while (pnode != NULL) {
        printf("userName: %s Message: %s\n", pnode->userName, pnode->Message);
        pnode = pnode->next;
    }
}

void screen_name(struct nk_context *ctx, AppState *s, int win_w, int win_h) {
    int panel_w = 300;
    int panel_h = 300;
    int panel_x = (win_w - panel_w) / 2;
    int panel_y = (win_h - panel_h) / 2;
    if (nk_begin(ctx, "Name Prompt",  nk_rect(panel_x, panel_y, panel_w, panel_h), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR) ){
        nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
        nk_layout_row_push(ctx, 80);
        nk_label(ctx, "Your Name", NK_TEXT_LEFT);

        nk_layout_row_push(ctx, 180);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, s->NameBuff, &s->name_len, NameSize, nk_filter_default);
        nk_layout_row_end(ctx);

        nk_layout_row_dynamic(ctx, 10, 1);
        nk_spacer(ctx);

        nk_layout_row_dynamic(ctx, 35, 1);
        if (nk_button_label(ctx, "Connect") && s->name_len > 0) {
            s->NameBuff[s->name_len] = '\0';
            s->Screen = SCREEN_CHAT;
            s->scroll_to_bottom = 1;
        }
    }
    nk_end(ctx);
}
void submit_message(AppState* s) {
    if (s->history_count >= MaxHistory) {
        return;
    }
    ChatLine *line = &s->history[s->history_count++];
    s->InputBuff[s->input_len] = '\0';
    snprintf(line->line, MaxLine, "%s: %s", s->NameBuff, s->InputBuff);
    line->is_mine = 1;

    s->input_len = 0;
    s->InputBuff[0] = '\0';

    s->scroll_to_bottom = 1;
}
void screen_chat(struct nk_context *ctx, AppState *s, int win_w, int win_h, int socketFD, struct messageNode** phead, struct messageNode** ptail) {
    if (nk_begin(ctx, "Chat Screen", nk_rect(0,0, win_w, win_h), NK_WINDOW_NO_SCROLLBAR) ) {
        float input_height = 50;
        float history_height = win_h - input_height - 50;
        nk_layout_row_dynamic(ctx, history_height, 1);
        nk_uint scroll_x = 0, scroll_y = 0;
        if (s->scroll_to_bottom) {
            scroll_y =0xFFFFFFFF;
            s->scroll_to_bottom = 0;
        }
        if (nk_group_scrolled_offset_begin(ctx, &scroll_x, &scroll_y, "history", NK_WINDOW_BORDER)) {
            struct messageNode* pnode = *phead;
            while (pnode != NULL) {
                nk_layout_row_dynamic(ctx, 20, 1);
                char line[1024];
                snprintf(line, 1024, "%s: %s", pnode->userName, pnode->Message);
                nk_label_colored(ctx, line, NK_TEXT_LEFT, nk_rgb(255, 255, 255));
                pnode = pnode->next;
            }

            nk_group_scrolled_end(ctx);
        }
        nk_layout_row_begin(ctx, NK_STATIC, input_height-10, 2);
        nk_layout_row_push(ctx,  win_w - 100 - 16);
        nk_flags result = nk_edit_string(ctx, NK_EDIT_SIMPLE, s->InputBuff, &s->input_len, MessageSize,nk_filter_default);
        nk_layout_row_push(ctx, 90);

        int send_clicked = nk_button_label(ctx, "Send");
        if ((result & NK_EDIT_COMMITED) || send_clicked) {
            if (s->input_len > 0) {
                sendmessage(socketFD, s->NameBuff, s->InputBuff);
                submit_message(s);
            }
        }
        nk_layout_row_end(ctx);
    }
    nk_end(ctx);
}





void window(int socketFD, struct messageNode** ptail, struct messageNode** phead) {
    SDL_Init(SDL_INIT_VIDEO);

    // 2. Create the window
    SDL_Window* win = SDL_CreateWindow(
        "ChatAppClient",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_SHOWN
    );
    if (!win) {
        SDL_Log("Window error: %s", SDL_GetError());
        return;
    }

    // 3. Create the renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Renderer error: %s", SDL_GetError());
        SDL_DestroyWindow(win);
        return;
    }

    // 4. Init Nuklear on top of SDL
    struct nk_context* ctx = nk_sdl_init(win, renderer);

    // 5. Load font (required or it will crash on render)
    struct nk_font_atlas* atlas;
    nk_sdl_font_stash_begin(&atlas);
    struct nk_font* font = nk_font_atlas_add_default(atlas, 14, NULL);
    nk_sdl_font_stash_end();
    nk_style_set_font(ctx, &font->handle);
    AppState state = {0};
    state.Screen = SCREEN_NAME;
    // 6. Main loop
    int running = 1;
    SDL_Event evt;
    while (running) {
        // Handle input
        nk_input_begin(ctx);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) running = 0;
            nk_sdl_handle_event(&evt);
        }
        nk_input_end(ctx);

        // Draw your UI here with nk_begin / nk_end etc.
        if (state.Screen == SCREEN_NAME) {
            screen_name(ctx, &state, WINDOW_W, WINDOW_H);
        }
        if (state.Screen == SCREEN_CHAT) {
            screen_chat(ctx, &state, WINDOW_W, WINDOW_H, socketFD, phead, ptail);
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        nk_sdl_render(NK_ANTI_ALIASING_ON);
        SDL_RenderPresent(renderer);
    }

    // 7. Cleanup
    nk_sdl_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

}
void* SyncThread(void* args) {
    struct SyncThreadStruct* SyncArgs = (struct SyncThreadStruct*)args;
    pthread_mutex_lock()
}
int main(void) {
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in ServerAddr;
    bzero(&ServerAddr, sizeof(ServerAddr));
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = inet_addr(IP);
    ServerAddr.sin_port = htons(PORT);

    pthread_mutex_t SocketMutex;
    pthread_mutex_init(&SocketMutex, NULL);


    struct messageNode* phead = NULL;
    struct messageNode* ptail = NULL;
    while (connect(socketFD, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr)));

    syncdata(socketFD, &ptail, &phead);

    window(socketFD, &ptail, &phead);
    disconnect(socketFD);
}