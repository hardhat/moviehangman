#include "sound.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct Note {
    uint16_t divider;
    uint16_t duration;
    sound_volume_t volume;
} Note;

typedef struct Song {
    sound_waveform_t waveform;
    const Note* notes;
    uint8_t length;
} Song;

#define NOTE(frequency, duration, volume) \
    {SOUND_FREQ_TO_DIV(frequency), duration, volume}
#define NOTE_COUNT(notes) (sizeof(notes) / sizeof((notes)[0]))

static const Note startup_notes[] = {
    NOTE(FREQ_C4, 90, VOL_75),
    NOTE(FREQ_E5, 90, VOL_75),
    NOTE(FREQ_G5, 110, VOL_100),
    NOTE(FREQ_C6, 220, VOL_100),
};

static const Note letter_correct_notes[] = {
    NOTE(FREQ_G5, 70, VOL_75),
    NOTE(FREQ_C6, 130, VOL_100),
};

static const Note letter_incorrect_notes[] = {
    NOTE(FREQ_A4, 120, VOL_100),
    NOTE(FREQ_E4, 220, VOL_75),
};

static const Note phrase_complete_notes[] = {
    NOTE(FREQ_C4, 90, VOL_75),
    NOTE(FREQ_E5, 90, VOL_75),
    NOTE(FREQ_G5, 90, VOL_100),
    NOTE(FREQ_C6, 180, VOL_100),
    NOTE(FREQ_E6, 260, VOL_100),
};

static const Note game_over_notes[] = {
    NOTE(FREQ_G4, 180, VOL_100),
    NOTE(FREQ_E4, 180, VOL_75),
    NOTE(FREQ_C4, 220, VOL_75),
    NOTE(FREQ_G3, 400, VOL_50),
};

static const Note move_cursor_notes[] = {
    NOTE(FREQ_C5, 50, VOL_75),
};

static const Song songs[] = {
    {WAV_SQUARE, NULL, 0},
    {WAV_SQUARE | DUTY_CYCLE_50_0, startup_notes, NOTE_COUNT(startup_notes)},
    {WAV_TRIANGLE, letter_correct_notes, NOTE_COUNT(letter_correct_notes)},
    {WAV_SAWTOOTH, letter_incorrect_notes, NOTE_COUNT(letter_incorrect_notes)},
    {WAV_SQUARE | DUTY_CYCLE_50_0, phrase_complete_notes, NOTE_COUNT(phrase_complete_notes)},
    {WAV_TRIANGLE, game_over_notes, NOTE_COUNT(game_over_notes)},
    {WAV_TRIANGLE, move_cursor_notes, NOTE_COUNT(move_cursor_notes)},
};

static uint8_t snd_playing_id;
static uint8_t snd_pos;
static uint16_t snd_timer;
static bool snd_playing;

static void sound_silence(void)
{
    snd_playing = false;
    snd_playing_id = 0;
    snd_pos = 0;
    snd_timer = 0;
    zvb_sound_set_hold(VOICE0, 1);
}

static void sound_start_note(void)
{
    const Song* current_song = &songs[snd_playing_id];
    const Note* current_note = &current_song->notes[snd_pos];

    zvb_sound_set_voices_vol(VOICE0, current_note->volume);
    zvb_sound_set_voices(VOICE0, current_note->divider, current_song->waveform);
    zvb_sound_set_hold(VOICE0, 0);
    snd_timer = current_note->duration;
}

void sound_init(void)
{
    zvb_sound_initialize(1);
    zvb_sound_set_channels(VOICE0, VOICE0);
    zvb_sound_set_volume(VOL_100);
    sound_silence();
}

void sound_play(uint8_t sound_id)
{
    if(sound_id == 0 || sound_id >= NOTE_COUNT(songs)) return;

    snd_playing_id = sound_id;
    snd_pos = 0;
    snd_playing = true;
    sound_start_note();
}

void sound_update(uint16_t delta_time)
{
    while(snd_playing && delta_time >= snd_timer) {
        delta_time -= snd_timer;
        snd_pos++;
        if(snd_pos >= songs[snd_playing_id].length) {
            sound_silence();
        } else {
            sound_start_note();
        }
    }

    if(snd_playing) snd_timer -= delta_time;
}

void sound_stop(uint8_t sound_id)
{
    if(snd_playing && sound_id == snd_playing_id) sound_silence();
}

void sound_term(void)
{
    sound_silence();
    zvb_sound_set_volume(VOL_0);
}