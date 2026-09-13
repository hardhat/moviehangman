#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <stdint.h>

#include <zvb_gfx.h>

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

enum TILESET_ID
{
    TILE_BACKGROUND = 0x00, // blank tile
    TILE_ALPHABET = 0x01, // row 0, column 1 == 'A'
    TILE_NUMBER = 0x70, // row 7, column 0 == '0'
    TILE_AVAILABLE_LETTER = 0x25, // yellow bg
    TILE_SELECTED_LETTER = 0x20, // pink bg
    TILE_USED_LETTER = 0x21, // purple bg
    TILE_SELECTED_USED_LETTER = 0x32, // black bg but already used
    TILE_HEAD = 0x40, // head 2x2
    TILE_LEFT_SHOE = 0x50, // left shoe 2x1
    TILE_RIGHT_SHOE = 0x52, // right show 2x1
    TILE_LEFT_ARM = 0x42, // left arm 2x1
    TILE_RIGHT_ARM = 0x44, // right arm 2x1
    TILE_HAND = 0x64, // hand
    TILE_LEG = 0x48, // leg square
    TILE_LEFT_HIP = 0x46, // left hip
    TILE_RIGHT_HIP = 0x47, // right hip
    TILE_SHIRT = 0x24, // shirt
    TILE_LEFT_SHOLDER = 0x52, // left shoulder
    TILE_RIGHT_SHOULDER = 0x53, // right shoulder
    TILE_CLUE = 0x4D, // blank clue tile 3x3
    TILE_CLUE_HIGHLIGHT = 0x2D, // highlighted clue tile 3x3
};

extern gfx_context ctx;
extern gfx_sprite sprites[128];
extern uint8_t next_sprite;

void debug_log(const char *message);
void debug_logf(const char *format, ...);


#endif // MAIN_H