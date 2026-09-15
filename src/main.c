// Copyright (c) 2026 Dale Wick
// SPDX-License-Identifier: MIT
// See LICENSE for the full license text.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <zos_sys.h>
#include <zos_keyboard.h>
#include <zos_errors.h>
#include <zos_vfs.h>
#include <zos_video.h>
#include <zvb_gfx.h>
#include <zvb_sound.h>
//#include <zvb_timer.h>

#include "main.h"
#include "img.h"
#include "game.h"

#ifndef __SDCC_VERSION_MAJOR
#define __at(addr)
#define __naked
#define __sfr
#define va_list struct {int dummy; }
#define va_start(ap, last)
#define va_end(ap)
#endif

gfx_context ctx;
bool done=false;

#define SCREEN_PALETTE_BASE 0x0000

gfx_sprite sprites[128];
uint8_t next_sprite = 0;

zos_dev_t ser;

void debug_log(const char *message)
{
    size_t size=strlen(message);

    write(ser, message, &size);
    size=2;
    write(ser, "\r\n", &size);
}

void debug_logf(const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    debug_log(buffer);
}

void init_graphics()
{
    ser = open("#SER0",O_WRONLY);
    if (ser < 0) {
        printf("Failed to open serial port\n");
	    exit(1);
    }
    debug_log("Initializing...");
    // Initialize the graphics context
    // Set to tiled 640x480 mode
    gfx_initialize(ZVB_CTRL_VID_MODE_GFX_640_8BIT, &ctx);

    gfx_palette_load(&ctx, tileset_palette, tileset_palette_len, SCREEN_PALETTE_BASE);

    for(int i=0;i<16;i++) {
        gfx_tileset_add_color_tile(&ctx, i, i | (i << 4));
    }
    gfx_tileset_add_color_tile(&ctx,255,0);
    for(int y=0;y<30;y++) {
        gfx_tilemap_load(&ctx, background_tilemap+40*y, 40, 0, 0, y);
        gfx_tilemap_load(&ctx, letterclue_tilemap+40*y, 40, 1, 0, y);
    }
    for(int y=0;y<30;y++) {
        for(int x=0;x<40;x++) {
            uint8_t tile = text_tilemap[y * 40 + x];
            if(tile!=255 && next_sprite<128) {
                sprites[next_sprite].tile = tile;
                sprites[next_sprite].x = 16+x*16;
                sprites[next_sprite].y = 16+y*16;
                sprites[next_sprite].flags = 0;
                sprites[next_sprite].options = 0;
                next_sprite++;
            }
        }
    }
    gfx_sprite_render_array(&ctx, 0, sprites, next_sprite);

    gfx_tileset_options options1={
        .from_byte = 0,
        .compression = TILESET_COMP_LZ,
        .opacity = 0,
        .pal_offset = SCREEN_PALETTE_BASE,
    };
    gfx_tileset_load(&ctx, tileset_tiles, tileset_tiles_len, &options1);
    debug_log("Graphics initialized.");
}

void send_input(uint8_t input, bool pressed)
{
    game_handle_input(input, pressed);
}

uint8_t handle_input(uint8_t key)
{
    switch(key)
    {
        case KB_ESC:
            done = true;
            return MAX_INPUT;
        case KB_KEY_W:
        case KB_UP_ARROW:
        case KB_NUMPAD_8:
            return INPUT_UP;
        case KB_KEY_S:
        case KB_DOWN_ARROW:
        case KB_NUMPAD_2:
            return INPUT_DOWN;
        case KB_KEY_A:
        case KB_LEFT_ARROW:
        case KB_NUMPAD_4:
            return INPUT_LEFT;
        case KB_KEY_D:
        case KB_RIGHT_ARROW:
        case KB_NUMPAD_6:
            return INPUT_RIGHT;
        case KB_KEY_V:
        case KB_KEY_SPACE:
            return INPUT_A;
        case KB_KEY_BACKSPACE:
        case KB_KEY_B:
            return INPUT_B;
        case KB_KEY_COMMA:
        case KB_KEY_X:
            return INPUT_X;
        case KB_KEY_PERIOD:
        case KB_KEY_Y:
            return INPUT_Y;
        case KB_KEY_ENTER:
            return INPUT_START;
        case KB_KEY_QUOTE:
        case KB_RIGHT_SHIFT:
            return INPUT_SELECT;
        case KB_KEY_LEFT_BRACKET:
        case KB_KEY_Q:
            return INPUT_L;
        case KB_KEY_RIGHT_BRACKET:
        case KB_KEY_E:
            return INPUT_R;
        default:
            return MAX_INPUT;
    }
}

void process_input()
{
    unsigned char keys[32];
    int size;
    bool pressed = true;

    do {
        size=32;
        read(DEV_STDIN, &keys, &size);
        if(size > 0) {
            debug_logf("Read %d keys from input.", size);
        }
        for(int i=0;i<size;i++) {
            char key = keys[i];
           if(key == KB_RELEASED) {
                pressed = false;
                debug_log("Processing released.");
            } else {
                debug_logf("Processing input key %02x.", key);
                uint8_t input = handle_input(key);
                if(input >= MAX_INPUT) {
                    pressed = true;
                    continue;
                }
                send_input(input, pressed);
                pressed=true;
            }
        }
    } while(size>0);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    init_graphics();

    /* Initialize the keyboard by setting it to raw and non-blocking */
    void* arg = (void*) (KB_READ_NON_BLOCK | KB_MODE_RAW);
    ioctl(DEV_STDIN, KB_CMD_SET_MODE, arg);

    game_reset();
    // Set the initial time to 0
    zos_time_t delta;
    delta.t_millis = 0;
    settime(0,&delta);
    // Main loop
    while (!done) {
        // Poll the keyboard for input
        gfx_wait_end_vblank(&ctx);
        process_input();
        //gettime(0,&delta);
        uint16_t elapsed = 16; //delta.t_millis;
        //if(elapsed==0) elapsed=16;
        //delta.t_millis = 0; // Reset the delta time for the next frame
        //settime(0,&delta);
        game_update(elapsed); // Use the actual elapsed time since the last frame
        gfx_wait_vblank(&ctx);
        game_draw();
    }

    zvb_sound_reset();
    memset(sprites, 0, sizeof(sprites));
    gfx_sprite_render_array(&ctx, 0, sprites, 128);
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
    exit(0);

    return 0;
}
