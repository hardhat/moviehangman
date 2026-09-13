#include <string.h>
#include <stdlib.h>

#include <zvb_gfx.h>

#include "main.h"
#include "movies.h"
#include "game.h"

uint16_t movie_year=0;
uint8_t phrase[32]; // Stores the current movie name for the round.
uint8_t letter_available[26]; // Tracks which letters are available for guessing.
uint8_t letter_count = 0;
uint8_t cursor = 0; // Tracks which letter is highlighted for the current round.
// Track the letters in the current phrase.
struct Clue {
    char letter; // The letter for this clue.
    uint8_t sprite_index;   // 255 (if unsolved) or the sprite index of the letter clue.
    uint8_t x,y;    // In tiles, where the 3x3 letter clue is positioned.
} clue[32];
uint8_t clue_count = 0; // Tracks the number of clues in the current phrase.

uint8_t add_sprite(uint8_t tile, uint16_t x, uint16_t y,uint8_t flags)
{
    sprites[next_sprite].tile = tile;
    sprites[next_sprite].x = x;
    sprites[next_sprite].y = y;
    sprites[next_sprite].flags = flags;
    sprites[next_sprite].options = 0;
    return next_sprite++;
}

void draw_available_letter_tile(uint8_t index, uint8_t tile)
{
    gfx_tilemap_place(&ctx, tile, 1, (index%7)*2+3, (index/7)*2+22);
}

void show_movie(void)
{
    // First word wrap the movie name.
    size_t len=strlen(phrase);
    uint8_t line[3]={0}; // length of each line. Assertion words can fit in 3 lines or less.
    uint8_t active_line=0;
    uint8_t line_start[3]={0}; // Tracks the starting index of each line.
    uint8_t last_word_end=0;    // save where on the line we were, to handle word wrapping correctly.
    uint8_t max_line=0; // Tracks the maximum line length for centering.
    uint8_t line_count; // The number of active lines filled in

    // Now build a list of lines word-wrapped within the 3-line limit.
    for(uint8_t i=0;i<len;i++) {
        if(phrase[i]==' ') {
            if(i-line_start[active_line]>14) {
                if(active_line>=3) break; // Prevent exceeding the 3-line limit.
                if(i-line_start[active_line]>max_line) max_line=i-line_start[active_line];
                line[active_line]=i-line_start[active_line];

                active_line++;
                line_start[active_line]=last_word_end;
            }
            last_word_end=i+1;
        }
    }
    // Grab last word of the phrase if it wasn't followed by a space.
    if(len - line_start[active_line] > 0) {
        if(len - line_start[active_line] > max_line) max_line = len - line_start[active_line];
        line[active_line] = len - line_start[active_line];
    }

    debug_logf("Phrase length: %d", len);
    debug_logf("Active line: %d", active_line);
    debug_logf("Line start indices: %d, %d, %d", line_start[0], line_start[1], line_start[2]);
    debug_logf("Line lengths: %d, %d, %d", line[0], line[1], line[2]);
    debug_logf("Max line length: %d", max_line);

    // Calculate where the centered tiles should go
    line_count=active_line+1;
    uint8_t left = 20-(max_line*3)/2;
    uint8_t original_left = left;
    uint8_t top = 9-(line_count*3)/2;
    clue_count=0;
    // Now draw the tiles for the 1, 2 or 3 lines:
    for(active_line=0;active_line<line_count;active_line++) {
        for(uint8_t letter=0;letter<line[active_line];letter++) {
            // Update clue structure and draw the clue tile.
            clue[clue_count].letter = phrase[line_start[active_line] + letter];
            clue[clue_count].sprite_index = 255; // unsolved
            clue[clue_count].x = left;
            clue[clue_count].y = top;
            clue_count++;
            if(phrase[line_start[active_line] + letter]!=' ') {
                uint8_t tiles0[3]={TILE_CLUE,TILE_CLUE+1,TILE_CLUE+2};
                gfx_tilemap_load(&ctx,tiles0,3,1,left,top);
                uint8_t tiles1[3]={TILE_CLUE+16,TILE_CLUE+17,TILE_CLUE+18};
                gfx_tilemap_load(&ctx,tiles1,3,1,left,top+1);
                uint8_t tiles2[3]={TILE_CLUE+32,TILE_CLUE+33,TILE_CLUE+34};
                gfx_tilemap_load(&ctx,tiles2,3,1,left,top+2);
            }
            left+=3;
        }
        left=original_left;
        top+=3;
    }
    debug_logf("Clue count after drawing: %d", clue_count);

    cursor=0;
    for(int i=0;i<26;i++) {
        if(i!=cursor) {
            draw_available_letter_tile(i, letter_available[i]?TILE_AVAILABLE_LETTER:TILE_USED_LETTER);
        }
    }
}

void game_handle_input(uint8_t input, bool pressed)
{
    if(phrase[0]==0) {
        // Time to pick a phrase after any input
        uint8_t index = rand() % movies_count;
        debug_logf("Selected movie index: %d", index);
        strncpy((char*)phrase, movies[index].title, sizeof(phrase));
        debug_logf("Selected movie title: %s", phrase);
        movie_year = movies[index].year;
        debug_logf("Selected movie year: %d", movie_year);
        show_movie();
        return;
    }
    if(input==INPUT_LEFT && pressed)  {
        draw_available_letter_tile(cursor,  letter_available[cursor]?TILE_AVAILABLE_LETTER:TILE_USED_LETTER);
        cursor=(26+cursor-1)%26;
        draw_available_letter_tile(cursor, letter_available[cursor]?TILE_SELECTED_LETTER:TILE_SELECTED_USED_LETTER);
    } else if(input==INPUT_RIGHT && pressed) {
        draw_available_letter_tile(cursor,  letter_available[cursor]?TILE_AVAILABLE_LETTER:TILE_USED_LETTER);
        cursor=(cursor+1)%26;
        draw_available_letter_tile(cursor, letter_available[cursor]?TILE_SELECTED_LETTER:TILE_SELECTED_USED_LETTER);
    } else if (input==INPUT_A && pressed) {
        // Handle selecting the current letter
        if(letter_available[cursor]) {
            letter_available[cursor] = 0;
            draw_available_letter_tile(cursor, TILE_USED_LETTER);
            // TODO: Check if the selected letter is in the phrase and update the clue tiles accordingly.
        }
    }
    
}

// Update the game state based on the elapsed time in ms (delta).
void game_update(uint16_t delta)
{
    (void)delta;
    // Seed random timer until the first phrase is set
    if(phrase[0]==0) rand();
}

void game_reset(void)
{
    memset(phrase, 0, sizeof(phrase));
    memset(letter_available, 1, sizeof(letter_available));
    letter_count = 0;
    cursor = 0;
    next_sprite = 0;
    memset(sprites, 0, sizeof(sprites));

    // Draw title
    const char *title_text="MOVIE HANGMAN";
    for(uint8_t i=0;i<strlen(title_text);i++) {
        if(title_text[i] == ' ') continue;
        add_sprite(TILE_ALPHABET+title_text[i]-'A', 3*16+i*16, 32, 0);
    }
    // Draw subtitle
    const char *subtitle_text="FROM";
    for(uint8_t i=0;i<strlen(subtitle_text);i++) {
        if(subtitle_text[i] == ' ') continue;
        add_sprite(TILE_ALPHABET+subtitle_text[i]-'A', 3*16+i*16, 64, 0);
    }
    // Label all of the alphabet letters for guessing
    for(uint8_t i = 0; i < 26; i++) {
        add_sprite(TILE_ALPHABET+i, 64+(i%7)*32, (i/7)*32+22*16+16, 0);
        draw_available_letter_tile(i, TILE_AVAILABLE_LETTER);
    }
    draw_available_letter_tile(26, TILE_AVAILABLE_LETTER+1);
    draw_available_letter_tile(27, TILE_AVAILABLE_LETTER+1);
    gfx_sprite_render_array(&ctx, 0, sprites, 128);

    for(uint8_t i = 0; i < clue_count; i++) {
        clue[i].letter = 0;
        clue[i].sprite_index = 255; // unsolved
        clue[i].x = 0;
        clue[i].y = 0;
    }
    clue_count = 0;

    // Display alphabet availability for guessing
    for(uint8_t i = 0; i < 26; i++) {
        draw_available_letter_tile(i, TILE_AVAILABLE_LETTER);
    }
    uint8_t man[6]={0};
    // Clear man from scaffold
    for(uint8_t y=16;y<12;y++) {
        gfx_tilemap_load(&ctx,man,6,1,32,y);
    }
    uint8_t line[40]={0};
    // Clear solution tiles
    for(uint8_t y=4;y<14;y++) {
        gfx_tilemap_load(&ctx,line,40,1,0,y);
    }
}

void game_draw(void)
{
}
