#include <stdio.h>
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
#define CLUE_COUNT 32
struct Clue {
    char letter; // The letter for this clue.
    uint8_t sprite_index;   // 255 (if unsolved) or the sprite index of the letter clue.
    uint8_t x,y;    // In tiles, where the 3x3 letter clue is positioned.
} clue[CLUE_COUNT];
uint8_t clue_count = 0; // Tracks the number of clues in the current phrase.
#define ANIMATED_LETTER_COUNT 32
#define ANIMATED_SPEED 10
struct AnimatedLetter {
    char letter; // The letter for this animated clue
    uint8_t sprite_index;   // The sprite index of the animated letter clue or 255 if inactive.
    uint16_t target_x,target_y;    // In pixels, where the 3x3 letter clue is positioned.
    int16_t delta;
    int16_t delta_x,delta_y;
    int8_t step_x,step_y;
} animated_letter[ANIMATED_LETTER_COUNT];

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
    // Word wrap the movie name into up to 3 centered lines.
    size_t len=strlen(phrase);
    uint8_t line[3]={0}; // length of each line. Assertion words can fit in 3 lines or less.
    uint8_t active_line=0;
    uint8_t line_start[3]={0}; // Tracks the starting index of each line.
    uint8_t max_line=0; // Tracks the maximum line length for centering.
    uint8_t line_count=1; // The number of active lines filled in

    uint8_t word_start=0; // Start index of the word currently being scanned.
    uint8_t cur_len=0;    // Length of content accumulated on the current line so far.

    // Walk the phrase a word at a time (a word ends at a space or the end of the string),
    // only wrapping to a new line when the word being added would overflow the current one.
    for(uint8_t i=0;i<=len;i++) {
        if(i==len || phrase[i]==' ') {
            uint8_t word_len=i-word_start;
            uint8_t sep=(cur_len>0)?1:0; // space needed before this word if the line isn't empty
            if(cur_len>0 && line_count<3 && cur_len+sep+word_len>12) {
                line[line_count-1]=cur_len;
                if(cur_len>max_line) max_line=cur_len;
                line_count++;
                line_start[line_count-1]=word_start;
                cur_len=word_len;
            } else {
                cur_len+=sep+word_len;
            }
            word_start=i+1;
        }
    }
    // Finalize the last line.
    line[line_count-1]=cur_len;
    if(cur_len>max_line) max_line=cur_len;

    debug_logf("Phrase length: %d", len);
    debug_logf("Line count: %d", line_count);
    debug_logf("Line start indices: %d, %d, %d", line_start[0], line_start[1], line_start[2]);
    debug_logf("Line lengths: %d, %d, %d", line[0], line[1], line[2]);
    debug_logf("Max line length: %d", max_line);

    // Calculate where the centered tiles should go
    uint8_t left = 19-(max_line*3)/2;
    uint8_t original_left = left;
    uint8_t top = 8-(line_count*3)/2;
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

    // Show the year in row 4, column 8
    if(movie_year > 0) {
        char year_str[16];
        sprintf(year_str, "%04d", movie_year);
        debug_log(year_str);
        for(uint8_t i=0;i<strlen(year_str);i++) {
            add_sprite(TILE_NUMBER+year_str[i]-'0', 8*16+i*16, 4*16, 0);
            debug_logf("Added sprite for year digit: %c at position %d", year_str[i], i);
        }
    }
    gfx_sprite_render_array(&ctx, 0, sprites, next_sprite);
}

void animate_clue_tile_solution(struct Clue *clue_tile)
{
    // Find an available animated letter slot and initialize it for the clue tile solution.
    uint8_t i;
    for(i = 0; i < ANIMATED_LETTER_COUNT; i++) {
        if(animated_letter[i].sprite_index == 255) break;
    }
    if(i==ANIMATED_LETTER_COUNT) return; // animation overflow
    animated_letter[i].letter = clue_tile->letter;
    uint8_t index = clue_tile->letter-'A';
    // Start position for the animated letter sprite on the selection grid
    animated_letter[i].sprite_index = add_sprite(TILE_ALPHABET+clue_tile->letter-'A', 
        ((index%7)*2+3)*16+16, 
        ((index/7)*2+22)*16+16,
        0);
    animated_letter[i].target_x = clue_tile->x*16+32;
    animated_letter[i].target_y = clue_tile->y*16+32;
    uint8_t sprite_index = animated_letter[i].sprite_index;
    int16_t dx = animated_letter[i].target_x-sprites[sprite_index].x;
    int16_t dy = animated_letter[i].target_y-sprites[sprite_index].y;
    animated_letter[i].delta_x = dx<0?-dx:dx;
    animated_letter[i].delta_y = dy<0?dy:-dy;
    animated_letter[i].step_x = dx<0?-1:1;
    animated_letter[i].step_y = dy<0?-1:1;
    animated_letter[i].delta = animated_letter[i].delta_x+animated_letter[i].delta_y;

    uint8_t left = clue_tile->x;
    uint8_t top = clue_tile->y;
    uint8_t tiles0[3]={TILE_CLUE_HIGHLIGHT,TILE_CLUE_HIGHLIGHT+1,TILE_CLUE_HIGHLIGHT+2};
    gfx_tilemap_load(&ctx,tiles0,3,1,left,top);
    uint8_t tiles1[3]={TILE_CLUE_HIGHLIGHT+16,TILE_CLUE_HIGHLIGHT+17,TILE_CLUE_HIGHLIGHT+18};
    gfx_tilemap_load(&ctx,tiles1,3,1,left,top+1);
    uint8_t tiles2[3]={TILE_CLUE_HIGHLIGHT+32,TILE_CLUE_HIGHLIGHT+33,TILE_CLUE_HIGHLIGHT+34};
    gfx_tilemap_load(&ctx,tiles2,3,1,left,top+2);
}

void game_handle_input(uint8_t input, bool pressed)
{
    if(phrase[0]==0 || (input==INPUT_START && !pressed)) {
        // Time to pick a phrase after any input
        if(input==INPUT_START && !pressed) game_reset();
        uint8_t index = rand() % movies_count;
        debug_logf("Selected movie index: %d", index);
        strncpy((char*)phrase, movies[index].title, sizeof(phrase));
        debug_logf("Selected movie title: %s", phrase);
        movie_year = movies[index].year;
        debug_logf("Selected movie year: %d", movie_year);
        show_movie();
        draw_available_letter_tile(cursor, TILE_SELECTED_LETTER);
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
    } else if(input==INPUT_UP && pressed) {
        draw_available_letter_tile(cursor,  letter_available[cursor]?TILE_AVAILABLE_LETTER:TILE_USED_LETTER);
        cursor=(28+cursor-7)%28;
        if(cursor > 25) cursor -= 7;
        draw_available_letter_tile(cursor, letter_available[cursor]?TILE_SELECTED_LETTER:TILE_SELECTED_USED_LETTER);
    } else if(input==INPUT_DOWN && pressed) {
        draw_available_letter_tile(cursor,  letter_available[cursor]?TILE_AVAILABLE_LETTER:TILE_USED_LETTER);
        cursor=(cursor+7)%28;
        if(cursor > 25) cursor = cursor%7;
        draw_available_letter_tile(cursor, letter_available[cursor]?TILE_SELECTED_LETTER:TILE_SELECTED_USED_LETTER);
    } else if (input==INPUT_A && pressed) {
        // Handle selecting the current letter
        if(letter_available[cursor]) {
            letter_available[cursor] = 0;
            draw_available_letter_tile(cursor, TILE_USED_LETTER);
            // Check if the selected letter is in the phrase and 
            // animate the letter flying up to the clue tile.
            for(uint8_t i = 0; i < clue_count; i++) {
                if(clue[i].letter == ('A' + cursor)) {
                    animate_clue_tile_solution(&clue[i]);
                }
            }
        }
    }
    
}

// Update the game state based on the elapsed time in ms (delta).
void game_update(uint16_t delta)
{
    (void)delta;
    // Seed random timer until the first phrase is set
    if(phrase[0]==0) rand();

    for(uint8_t i = 0; i < ANIMATED_LETTER_COUNT; i++) {
        if(animated_letter[i].sprite_index != 255) {
            uint8_t sprite_index = animated_letter[i].sprite_index;
            for(uint8_t step = 0; step < ANIMATED_SPEED; step++) {
                if(sprites[sprite_index].x == animated_letter[i].target_x &&
                   sprites[sprite_index].y == animated_letter[i].target_y) {
                    animated_letter[i].sprite_index = 255;
                    break;
                }

                int16_t doubled_delta = 2*animated_letter[i].delta;
                if(doubled_delta >= animated_letter[i].delta_y) {
                    animated_letter[i].delta += animated_letter[i].delta_y;
                    sprites[sprite_index].x += animated_letter[i].step_x;
                }
                if(doubled_delta <= animated_letter[i].delta_x) {
                    animated_letter[i].delta += animated_letter[i].delta_x;
                    sprites[sprite_index].y += animated_letter[i].step_y;
                }
            }
        }
    }
}

void game_reset(void)
{
    memset(phrase, 0, sizeof(phrase));
    memset(letter_available, 1, sizeof(letter_available));
    letter_count = 0;
    cursor = 0;
    next_sprite = 0;
    memset(sprites, 0, sizeof(sprites));
    memset(animated_letter, 0xFF, sizeof(animated_letter)); // Mark all animated letters as inactive

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
    for(uint8_t y=16;y<16+12;y++) {
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
    gfx_sprite_render_array(&ctx, 0, sprites, 128);
}
