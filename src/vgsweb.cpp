#include <pthread.h>
#include <SDL2/SDL.h>
#include <emscripten.h>
#include <cstdlib>
#include <string>
#include "../vgszero/src/core/vgs0.hpp"
#include "gamepkg.h"

static pthread_mutex_t soundMutex = PTHREAD_MUTEX_INITIALIZER;
static VGS0 vgs0;

struct context
{
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    unsigned int *frameBuffer;
    int framePitch;
    int frameWidth;
    int frameHeight;
};

static void audioCallback(void* userdata, Uint8* stream, int len)
{
    pthread_mutex_lock(&soundMutex);
    void* buf = vgs0.tickSound(len);
    memcpy(stream, buf, len);
    pthread_mutex_unlock(&soundMutex);
}

static inline unsigned char bit5To8(unsigned char bit5)
{
    bit5 <<= 3;
    bit5 |= (bit5 & 0b11100000) >> 5;
    return bit5;
}

void mainloop(void *arg)
{
    context *ctx = static_cast<context *>(arg);

    // 入力処理
    static unsigned char joypad = 0;
    SDL_Event event;
    SDL_PollEvent(&event);
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_LEFT: joypad |= VGS0_JOYPAD_LE; break;
            case SDLK_RIGHT: joypad |= VGS0_JOYPAD_RI; break;
            case SDLK_UP: joypad |= VGS0_JOYPAD_UP; break;
            case SDLK_DOWN: joypad |= VGS0_JOYPAD_DW; break;
            case SDLK_x: joypad |= VGS0_JOYPAD_T1; break;
            case SDLK_z: joypad |= VGS0_JOYPAD_T2; break;
            case SDLK_SPACE: joypad |= VGS0_JOYPAD_ST; break;
            case SDLK_ESCAPE: joypad |= VGS0_JOYPAD_SE; break;
        }
    } else if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
            case SDLK_LEFT: joypad ^= VGS0_JOYPAD_LE; break;
            case SDLK_RIGHT: joypad ^= VGS0_JOYPAD_RI; break;
            case SDLK_UP: joypad ^= VGS0_JOYPAD_UP; break;
            case SDLK_DOWN: joypad ^= VGS0_JOYPAD_DW; break;
            case SDLK_x: joypad ^= VGS0_JOYPAD_T1; break;
            case SDLK_z: joypad ^= VGS0_JOYPAD_T2; break;
            case SDLK_SPACE: joypad ^= VGS0_JOYPAD_ST; break;
            case SDLK_ESCAPE: joypad ^= VGS0_JOYPAD_SE; break;
        }
    }

    // エミュレータ実行
    pthread_mutex_lock(&soundMutex);
    vgs0.tick(joypad);
    pthread_mutex_unlock(&soundMutex);
    unsigned short* vgsDisplay = vgs0.getDisplay();
    unsigned int* pcDisplay = ctx->frameBuffer;
    for (int y = 0; y < 192; y++) {
        for (int x = 0; x < 240; x++, pcDisplay += 2, vgsDisplay++) {
            unsigned int rgb555 = *vgsDisplay;
            unsigned int rgb888 = 0;
            rgb888 |= bit5To8((rgb555 & 0b0111110000000000) >> 10);
            rgb888 <<= 8;
            rgb888 |= bit5To8((rgb555 & 0b0000001111100000) >> 5);
            rgb888 <<= 8;
            rgb888 |= bit5To8(rgb555 & 0b0000000000011111);
            rgb888 <<= 8;
            pcDisplay[0] = rgb888;
            pcDisplay[1] = rgb888 & 0xF0F0F0F0;
            pcDisplay[ctx->frameWidth] = rgb888 & 0x8F8F8F8F;
            pcDisplay[ctx->frameWidth + 1] = rgb888 & 0x80808080;
        }
        pcDisplay += ctx->frameWidth;
    }

    // 画面更新処理
    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
    SDL_UpdateTexture(ctx->texture, nullptr, ctx->frameBuffer, ctx->framePitch);
    SDL_SetRenderTarget(ctx->renderer, nullptr);
    SDL_RenderCopy(ctx->renderer, ctx->texture, nullptr, nullptr);
    SDL_RenderPresent(ctx->renderer);
}

int main()
{
    // VGS-Zeroを初期化
    int romSize;
    const void* rom = nullptr;
    int bgmSize;
    const void* bgm = nullptr;
    int seSize;
    const void* se = nullptr;
    const unsigned char* ptr = rom_GAMEPKG;
    if (0 != memcmp(ptr, "VGS0PKG", 8)) {
        return -1;
    }
    ptr += 8;
    memcpy(&romSize, ptr, 4);
    ptr += 4;
    rom = ptr;
    ptr += romSize;
    if (romSize < 8 + 8192) {
        exit(-1);
    }
    memcpy(&bgmSize, ptr, 4);
    ptr += 4;
    bgm = 0 < bgmSize ? ptr : nullptr;
    ptr += bgmSize;
    memcpy(&seSize, ptr, 4);
    ptr += 4;
    se = 0 < seSize ? ptr : nullptr;

    if (0 < bgmSize) vgs0.loadBgm(bgm, bgmSize);
    if (0 < seSize) vgs0.loadSoundEffect(se, seSize);
    vgs0.loadRom(rom, romSize);

    // SDL（映像）を初期化
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    int screenWidth, screenHeight;
    emscripten_get_screen_size(&screenWidth, &screenHeight);
    screenWidth *= 60;
    screenWidth /= 100;
    int width = 480;
    int height = 384;
    int windowWidth;
    int windowHeight;
    if (width < screenWidth) {
        windowWidth = width;
        windowHeight = height;
    } else {
        double scale = height;
        scale /= width;
        windowWidth = screenWidth;
        windowHeight = (int)(screenWidth * scale);
    }
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(windowWidth, windowHeight, 0, &window, &renderer);
    SDL_RenderSetLogicalSize(renderer, width, height);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    context ctx;
    ctx.renderer = renderer;
    ctx.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    ctx.frameBuffer = (unsigned int*)malloc(width * height * 4);
    ctx.framePitch = width * 4;
    ctx.frameWidth = width;
    ctx.frameHeight = height;

    // SDL（音声）を初期化
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    desired.freq = 44100;
    desired.format = AUDIO_S16LSB;
    desired.channels = 1;
    desired.samples = 4096;
    desired.callback = audioCallback;
    desired.userdata = nullptr;
    auto audioDeviceId = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    SDL_PauseAudioDevice(audioDeviceId, 0);

    // メインループを 60fps で実行
    const int simulate_infinite_loop = 1;
    const int fps = 0;
    emscripten_set_main_loop_arg(mainloop, &ctx, fps, simulate_infinite_loop);

    // 終了処理
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
