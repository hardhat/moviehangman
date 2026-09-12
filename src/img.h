// Copyright (c) 2026 Dale Wick
// SPDX-License-Identifier: MIT
// See LICENSE.md for the full license text.

#ifndef IMG_H
#define IMG_H

#include <stdint.h>

extern const uint16_t tileset_palette[64];
extern const uint16_t tileset_palette_len;
#define tileset_PALETTE_BASE 0
extern const uint8_t *tileset_tiles;
#define tileset_TILES_BASE 0
extern const uint16_t tileset_tiles_len;
extern const uint8_t *background_tilemap;
extern const uint8_t *letterclue_tilemap;
extern const uint8_t *text_tilemap;

#endif // IMG_H