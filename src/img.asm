; Copyright (c) 2026 Dale Wick
; SPDX-License-Identifier: MIT
; See LICENSE for the full license text.

; Export the symbols

  .module img
  .area _TEXT

	.globl _tileset_palette
    .globl _tileset_palette_len
	.globl _tileset_tiles
    .globl _tileset_tiles_len
	.globl _background_tilemap
    .globl _letterclue_tilemap
    .globl _text_tilemap

_tileset_palette:
    .incbin "img/tileset.ztp"
_tileset_palette_len:
    .dw .-_tileset_palette
_tileset_tiles:
    .dw _tileset_tiles_data
_tileset_tiles_data:
    .incbin "img/tileset.zts"
_tileset_tiles_len:
    .dw .-_tileset_tiles_data

_background_tilemap:
    .dw _background_tilemap_data
_background_tilemap_data:
    .incbin "map/background.ztm"
_letterclue_tilemap:
    .dw _letterclue_tilemap_data
_letterclue_tilemap_data:
    .incbin "map/letterclue.ztm"
_text_tilemap:
    .dw _text_tilemap_data
_text_tilemap_data:
    .incbin "map/text.ztm"

