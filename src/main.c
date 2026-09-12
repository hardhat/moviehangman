// Copyright (c) 2026 Dale Wick
// SPDX-License-Identifier: MIT
// See LICENSE for the full license text.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <zos_sys.h>
#include <zos_keyboard.h>
#include <zos_errors.h>
#include <zos_vfs.h>
#include <zos_video.h>
#include <zvb_gfx.h>
#include <zvb_sound.h>

#include "img.h"

enum INPUT
{
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_A,
    INPUT_B,
    INPUT_X,
    INPUT_Y,
    INPUT_START,
    INPUT_SELECT,
    INPUT_L,
    INPUT_R,
    MAX_INPUT
};

gfx_context ctx;
bool done=false;

#define SCREEN_PALETTE_BASE 0x0000
const uint16_t screen_palette[16]={
    (uint16_t)RGB888_TO_RGB565(0x3a, 0x25, 0x25), // 0  Dark brown - outlines
    (uint16_t)RGB888_TO_RGB565(0xff, 0xff, 0xff), // 1  White - clouds/highlights
    (uint16_t)RGB888_TO_RGB565(0x6f, 0xd4, 0xed), // 2  Sky blue - background
    (uint16_t)RGB888_TO_RGB565(0xb8, 0xec, 0xf5), // 3  Light cyan - sky highlights
    (uint16_t)RGB888_TO_RGB565(0x08, 0x8f, 0xd1), // 4  Dark blue - UI shadows
    (uint16_t)RGB888_TO_RGB565(0x13, 0xb5, 0xe8), // 5  Bright blue - buttons
    (uint16_t)RGB888_TO_RGB565(0x32, 0xc1, 0x70), // 6  Green - grass
    (uint16_t)RGB888_TO_RGB565(0x16, 0x9b, 0x55), // 7  Dark green - foliage
    (uint16_t)RGB888_TO_RGB565(0xff, 0xd3, 0x43), // 8  Yellow - stars/score
    (uint16_t)RGB888_TO_RGB565(0xff, 0x98, 0x18), // 9  Orange - buttons/highlights
    (uint16_t)RGB888_TO_RGB565(0xf2, 0x5b, 0x2a), // 10 Red-orange - danger
    (uint16_t)RGB888_TO_RGB565(0xb8, 0x6b, 0x32), // 11 Brown - wood/earth
    (uint16_t)RGB888_TO_RGB565(0x73, 0x3d, 0x2c), // 12 Dark brown - deep earth/shadows
    (uint16_t)RGB888_TO_RGB565(0xf0, 0x6f, 0xa8), // 13 Pink - character accents
    (uint16_t)RGB888_TO_RGB565(0x8e, 0x65, 0xc9), // 14 Purple - special UI
    (uint16_t)RGB888_TO_RGB565(0xff, 0xd9, 0x9b), // 15 Cream/skin - character
};

gfx_sprite sprites[128];
uint8_t next_sprite = 0;

void init_graphics()
{
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
}

void send_input(uint8_t input, bool pressed)
{
    if(input==MAX_INPUT) {
        done=true;
        return;
    }
    if(pressed)
        return;
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
            return INPUT_UP;
        case KB_KEY_S:
        case KB_DOWN_ARROW:
            return INPUT_DOWN;
        case KB_KEY_A:
        case KB_LEFT_ARROW:
            return INPUT_LEFT;
        case KB_KEY_D:
        case KB_RIGHT_ARROW:
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
        for(int i=0;i<size;i++) {
            char key = keys[i];
           //debug_logf("Processing input key %02x.", key);
           if(key == KB_RELEASED) {
                pressed = false;
            } else {
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

    // Main loop
    while (!done) {
        // Poll the keyboard for input
        process_input();
    }

    zvb_sound_reset();
    memset(sprites, 0, sizeof(sprites));
    gfx_sprite_render_array(&ctx, 0, sprites, 128);
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
    exit(0);

    return 0;
}