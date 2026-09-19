#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <zvb_gfx.h>

#include "main.h"
#include "movies.h"
#include "game.h"

uint16_t movie_year=0;
uint8_t phrase[32]; // Stores the current movie name for the round.
#define AVAILABLE_CHARACTER_COUNT 36
#define AVAILABLE_CHARACTER_COLUMNS 9
uint8_t letter_available[AVAILABLE_CHARACTER_COUNT]; // Tracks which letters and digits are available for guessing.
uint8_t letter_count = 0;
uint8_t unsolved_count = 0; // Tracks the number of unsolved letters in the current phrase.
uint8_t cursor = 0; // Tracks which letter or digit is highlighted for the current round.
bool won=false; // Show fireworks when the game is won
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
int random_seed=0;
#define FIREWORK_COUNT 32
#define GRAVITY 1
#define FIREWORK_INITIAL_ACCEL_Y -5
struct Fireworks {
    uint8_t sprite_index;   // The sprite index of the firework or 255 if inactive.
    uint16_t x, y;          // In pixels, the position of the firework.
    int16_t step_x, step_y; // The velocity of the firework.
    int8_t accel_y;       // The acceleration in the y direction.
    uint8_t delay;       // The remaining lifetime of the firework.
} fireworks[FIREWORK_COUNT]; // Array to hold multiple fireworks, adjust the size as needed.

uint8_t add_sprite(uint8_t tile, uint16_t x, uint16_t y,uint8_t flags)
{
    if(next_sprite>=128) return 255;
    sprites[next_sprite].tile = tile;
    sprites[next_sprite].x = x;
    sprites[next_sprite].y = y;
    sprites[next_sprite].flags = flags;
    sprites[next_sprite].options = 0;
    return next_sprite++;
}

uint8_t find_sprite(uint8_t tile, uint16_t x, uint16_t y,uint8_t flags)
{
    uint8_t sprite_index;
    // Look for an inactive sprite that can be reused.
    for(sprite_index=0;sprite_index<next_sprite;sprite_index++) {
        if(sprites[sprite_index].tile == 0 &&
           sprites[sprite_index].x == 0 &&
           sprites[sprite_index].y == 0) {
            sprites[sprite_index].tile = tile;
            sprites[sprite_index].x = x;
            sprites[sprite_index].y = y;
            sprites[sprite_index].flags = flags;
            sprites[sprite_index].options = 0;
            return sprite_index;
        }
    }
    // None available, so add one, if possible
    return add_sprite(tile, x, y, flags);
}

uint8_t character_tile(char character)
{
    if(character >= 'A' && character <= 'Z') {
        return TILE_ALPHABET + character - 'A';
    }
    if(character >= '0' && character <= '9') {
        return TILE_NUMBER + character - '0';
    }
    return TILE_BACKGROUND;
}

uint8_t character_index(char character)
{
    if(character >= 'A' && character <= 'Z') {
        return character - 'A';
    }
    if(character >= '0' && character <= '9') {
        return 26 + character - '0';
    }
    return 255; // Invalid character
}

char available_character(uint8_t index)
{
    if(index < 26) return 'A' + index;
    return '0' + index - 26;
}

void draw_available_letter_tile(uint8_t index, uint8_t tile)
{
    gfx_tilemap_place(&ctx, tile, 1,
        (index%AVAILABLE_CHARACTER_COLUMNS)*2+3,
        (index/AVAILABLE_CHARACTER_COLUMNS)*2+22);
}

void draw_key_bindings(const char *bindings)
{
    uint8_t buf[40] = {0};
    for(uint8_t i=0;i<strlen(bindings);i++) {
        if(bindings[i] == ' ') continue;
        switch(bindings[i]) {
            case '<':
                buf[i] = TILE_LESS_THAN; // Assuming TILE_LESS_THAN is defined for the '<' character
                break;
            case '>':
                buf[i] = TILE_GREATER_THAN; // Assuming TILE_GREATER_THAN is defined for the '>' character
                break;
            default:
                buf[i] = character_tile(bindings[i]);
                break;
        }
    }
    gfx_tilemap_load(&ctx, buf, 40, 1, 1, 4);
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
    uint8_t letters_present=0;  // Number of letters in the phrase.
    uint8_t numbers_present=0;  // Number of numbers in the phrase.

    // Walk the phrase a word at a time (a word ends at a space or the end of the string),
    // only wrapping to a new line when the word being added would overflow the current one.
    for(uint8_t i=0;i<=len;i++) {
        if(phrase[i] >= 'A' && phrase[i] <= 'Z') letters_present++;
        if(phrase[i] >= '0' && phrase[i] <= '9') numbers_present++;
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
    unsolved_count = clue_count; // Initialize the unsolved count to the total number of clues.
    // Finalize the last line.
    line[line_count-1]=cur_len;
    if(cur_len>max_line) max_line=cur_len;

    // debug_logf("Phrase length: %d", len);
    // debug_logf("Line count: %d", line_count);
    // debug_logf("Line start indices: %d, %d, %d", line_start[0], line_start[1], line_start[2]);
    // debug_logf("Line lengths: %d, %d, %d", line[0], line[1], line[2]);
    // debug_logf("Max line length: %d", max_line);

    // Calculate where the centered tiles should go
    uint8_t left = 19-(max_line*3)/2;
    uint8_t original_left = left;
    uint8_t top = 9-(line_count*3)/2;
    clue_count=0;
    // Now draw the tiles for the 1, 2 or 3 lines:
    for(active_line=0;active_line<line_count;active_line++) {
        for(uint8_t letter=0;letter<line[active_line];letter++) {
            // Update clue structure and draw the clue tile.
            if(phrase[line_start[active_line] + letter] == ' ') {
                left += 3;
                continue;
            }
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
    // debug_logf("Clue count after drawing: %d", clue_count);

    cursor=0;
    for(int i=0;i<AVAILABLE_CHARACTER_COUNT;i++) {
        if(i!=cursor) {
            draw_available_letter_tile(i, letter_available[i]?TILE_AVAILABLE_LETTER:TILE_USED_LETTER);
        }
        if(i<26 && !letters_present) {
            letter_available[i] = 0;
            draw_available_letter_tile(i, TILE_USED_LETTER);
        }
        if(i>=26 && !numbers_present) {
            letter_available[i] = 0;
            draw_available_letter_tile(i, TILE_USED_LETTER);
        }
    }

    draw_key_bindings("USE < > AND SPACE OR ENTER TO RESTART");
    // Draw subtitle
    const char *subtitle_text="FROM";
    for(uint8_t i=0;i<strlen(subtitle_text);i++) {
        if(subtitle_text[i] == ' ') continue;
        add_sprite(character_tile(subtitle_text[i]), 3*16+i*16, 48, 0);
    }
    // Show the year in row 4, column 8
    if(movie_year > 0) {
        char year_str[16];
        sprintf(year_str, "%04d", movie_year);
        //debug_log(year_str);
        for(uint8_t i=0;i<strlen(year_str);i++) {
            add_sprite(character_tile(year_str[i]), 8*16+i*16, 3*16, 0);
            // debug_logf("Added sprite for year digit: %c at position %d", year_str[i], i);
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
    uint8_t index = clue_tile->letter >= 'A' && clue_tile->letter <= 'Z'
        ? clue_tile->letter-'A' : clue_tile->letter-'0'+26;
    // Start position for the animated letter sprite on the selection grid
    animated_letter[i].sprite_index = add_sprite(character_tile(clue_tile->letter),
        ((index%AVAILABLE_CHARACTER_COLUMNS)*2+3)*16+16,
        ((index/AVAILABLE_CHARACTER_COLUMNS)*2+22)*16+16,
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
        if(phrase[0]==0) {
            srand(random_seed);
            random_seed++;
        }
        // Time to pick a phrase after any input
        if(input==INPUT_START && !pressed) game_reset();
        uint8_t index = (rand()>>8) % movies_count;
        // debug_logf("Selected movie index: %d", index);
        strncpy((char*)phrase, movies[index].title, sizeof(phrase));
        // debug_logf("Selected movie title: %s", phrase);
        movie_year = movies[index].year;
        // debug_logf("Selected movie year: %d", movie_year);
        show_movie();
        draw_available_letter_tile(cursor, TILE_SELECTED_LETTER);
        return;
    }
    if(input==INPUT_LEFT && pressed)  {
        draw_available_letter_tile(cursor,  letter_available[cursor]?TILE_AVAILABLE_LETTER:TILE_USED_LETTER);
        cursor=(AVAILABLE_CHARACTER_COUNT+cursor-1)%AVAILABLE_CHARACTER_COUNT;
        draw_available_letter_tile(cursor, letter_available[cursor]?TILE_SELECTED_LETTER:TILE_SELECTED_USED_LETTER);
    } else if(input==INPUT_RIGHT && pressed) {
        draw_available_letter_tile(cursor,  letter_available[cursor]?TILE_AVAILABLE_LETTER:TILE_USED_LETTER);
        cursor=(cursor+1)%AVAILABLE_CHARACTER_COUNT;
        draw_available_letter_tile(cursor, letter_available[cursor]?TILE_SELECTED_LETTER:TILE_SELECTED_USED_LETTER);
    } else if(input==INPUT_UP && pressed) {
        draw_available_letter_tile(cursor,  letter_available[cursor]?TILE_AVAILABLE_LETTER:TILE_USED_LETTER);
        cursor=(AVAILABLE_CHARACTER_COUNT+cursor-AVAILABLE_CHARACTER_COLUMNS)%AVAILABLE_CHARACTER_COUNT;
        draw_available_letter_tile(cursor, letter_available[cursor]?TILE_SELECTED_LETTER:TILE_SELECTED_USED_LETTER);
    } else if(input==INPUT_DOWN && pressed) {
        draw_available_letter_tile(cursor,  letter_available[cursor]?TILE_AVAILABLE_LETTER:TILE_USED_LETTER);
        cursor=(cursor+AVAILABLE_CHARACTER_COLUMNS)%AVAILABLE_CHARACTER_COUNT;
        draw_available_letter_tile(cursor, letter_available[cursor]?TILE_SELECTED_LETTER:TILE_SELECTED_USED_LETTER);
    } else if (input==INPUT_A && pressed) {
        // Handle selecting the current letter
        if(letter_available[cursor]) {
            letter_available[cursor] = 0;
            draw_available_letter_tile(cursor, TILE_USED_LETTER);
            // Check if the selected letter is in the phrase and 
            // animate the letter flying up to the clue tile.
            unsolved_count=0;
            for(uint8_t i = 0; i < clue_count; i++) {
                if(clue[i].letter == available_character(cursor)) {
                    animate_clue_tile_solution(&clue[i]);
                } else if(clue[i].letter != 0 && letter_available[character_index(clue[i].letter)]) {
                    unsolved_count++;
                    //debug_logf("Letter %d:%c is still unsolved.", i, clue[i].letter);
                }
            }
            //debug_logf("Unsolved count: %d", unsolved_count);
            if(unsolved_count == 0) {
                won = true;
                //debug_log("You won!");
            }
        }
    }
    
}

// Update the game state based on the elapsed time in ms (delta).
void game_update(uint16_t delta)
{
    (void)delta;
    // Seed random timer until the first phrase is set
    if(phrase[0]==0) {
        random_seed++;
    }

    for(uint8_t i = 0; i < ANIMATED_LETTER_COUNT; i++) {
        if(animated_letter[i].sprite_index != 255) {
            uint8_t sprite_index = animated_letter[i].sprite_index;
            for(uint8_t step = 0; step < ANIMATED_SPEED; step++) {
                if(sprites[sprite_index].x == animated_letter[i].target_x &&
                   sprites[sprite_index].y == animated_letter[i].target_y) {
                    animated_letter[i].sprite_index = 255;
                    unsolved_count--;
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
    if(won) {
        // Update fireworks for celebration
        for(uint8_t i = 0; i < FIREWORK_COUNT; i++) {
            if( fireworks[i].sprite_index == 255 && fireworks[i].delay == 0) {
                fireworks[i].x = ((rand()>>6)&511) + 320-256; // Random x position for the firework
                fireworks[i].y = (rand()>>8) % 128; // Random y position for the firework
                fireworks[i].step_x = (rand() % 9) - 4; // Random horizontal step for the firework
                fireworks[i].step_y = 1; // Random vertical step for the firework
                fireworks[i].accel_y = FIREWORK_INITIAL_ACCEL_Y; // Initial acceleration in the y direction
                uint8_t tile = (rand() & 1) ? TILE_FIREWORK : TILE_FIREWORK_2;
                fireworks[i].sprite_index = find_sprite(tile, fireworks[i].x, fireworks[i].y, rand()&6);
            } else if(fireworks[i].delay == 0) {
                fireworks[i].x += fireworks[i].step_x;
                fireworks[i].y += fireworks[i].step_y;
                fireworks[i].step_y += fireworks[i].accel_y;
                fireworks[i].accel_y += GRAVITY;
                // Alternate between the two firework tiles for animation
                gfx_sprite *sprite = &sprites[fireworks[i].sprite_index];
                //sprite->tile = TILE_FIREWORK+TILE_FIREWORK_2-sprite->tile;
                sprite->x = fireworks[i].x;
                sprite->y = fireworks[i].y;
                if(fireworks[i].x >= 640 || (fireworks[i].y >= 290 && fireworks[i].y <1024)) {
                    // Reset the firework if it goes out of bounds
                    sprite->x = 0;
                    sprite->y = 0;
                    sprite->tile = 0;
                    fireworks[i].step_x = 0;
                    fireworks[i].step_y = 0;
                    fireworks[i].accel_y = 0;
                    fireworks[i].sprite_index = 255;
                    fireworks[i].delay = (rand() & 63) + 5; // Random delay before the firework starts moving
                }
            } else {
                fireworks[i].delay--;
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
    unsolved_count = 0;
    won = false;
    next_sprite = 0;
    memset(sprites, 0, sizeof(sprites));
    memset(animated_letter, 0xFF, sizeof(animated_letter)); // Mark all animated letters as inactive

    draw_key_bindings("HIT SPACE TO START GAME");
    // Draw title
    const char *title_text="MOVIE HANGMAN";
    for(uint8_t i=0;i<strlen(title_text);i++) {
        if(title_text[i] == ' ') continue;
        add_sprite(character_tile(title_text[i]), 3*16+i*16, 16, 0);
    }
    // Label all of the alphabet letters and digits for guessing
    for(uint8_t i = 0; i < AVAILABLE_CHARACTER_COUNT; i++) {
        add_sprite(character_tile(available_character(i)),
            64+(i%AVAILABLE_CHARACTER_COLUMNS)*32,
            (i/AVAILABLE_CHARACTER_COLUMNS)*32+22*16+16, 0);
        draw_available_letter_tile(i, TILE_AVAILABLE_LETTER);
    }
    gfx_sprite_render_array(&ctx, 0, sprites, 128);

    for(uint8_t i = 0; i < clue_count; i++) {
        clue[i].letter = 0;
        clue[i].sprite_index = 255; // unsolved
        clue[i].x = 0;
        clue[i].y = 0;
    }
    clue_count = 0;

    // Display alphabet availability for guessing
    for(uint8_t i = 0; i < AVAILABLE_CHARACTER_COUNT; i++) {
        draw_available_letter_tile(i, TILE_AVAILABLE_LETTER);
    }
    uint8_t man[6]={0};
    // Clear man from scaffold
    for(uint8_t y=16;y<16+11;y++) {
        gfx_tilemap_load(&ctx,man,6,1,32,y);
    }
    uint8_t line[40]={0};
    // Clear solution tiles
    for(uint8_t y=5;y<13;y++) {
        gfx_tilemap_load(&ctx,line,40,1,0,y);
    }
    // Trigger fireworks or any other celebration for solving the puzzle
    for(uint8_t i = 0; i < FIREWORK_COUNT; i++) {
        fireworks[i].sprite_index = 255; // Mark all fireworks as inactive initially
    }
}

void game_draw(void)
{
    gfx_sprite_render_array(&ctx, 0, sprites, 128);
}
