/*
 * musicalsp430.h
 * various music related functions and data structures for microtracker
 * Josh Chandler
*/

#ifndef MUSICALSP430_H
#define MUSICALSP430_H

// TYPES

typedef unsigned int Slice;

enum NoteHertz // 12 * 4 = 48 notes
{
    C2  =  65, Cs2 =  69, D2  =  73, Ds2 =  78, E2  =  82, F2  =  87, // 2
    Fs2 =  93, G2  =  98, Gs2 = 104, A2  = 110, As2 = 117, B2  = 123, // 2
    C3  = 131, Cs3 = 139, D3  = 147, Ds3 = 156, E3  = 165, F3  = 175, // 3
    Fs3 = 185, G3  = 196, Gs3 = 208, A3  = 220, As3 = 233, B3  = 247, // 3
    C4  = 262, Cs4 = 277, D4  = 294, Ds4 = 311, E4  = 330, F4  = 349, // 4
    Fs4 = 370, G4  = 392, Gs4 = 415, A4  = 440, As4 = 466, B4  = 494, // 4
    C5  = 523, Cs5 = 554, D5  = 587, Ds5 = 622, E5  = 659, F5  = 698, // 5
    Fs5 = 740, G5  = 784, Gs5 = 831, A5  = 880, As5 = 932, B5  = 988  // 5
};
typedef enum NoteHertz MusicNote;

// GLOBALS
// because I'm a savage

#define BLOCK_SIZE (64)

Slice *Slice_buff; // global slice buffer. 32 slices.
int Slice_index = -1;   // Slice_index in Slice array

long unsigned int TICKS_PER_BEAT = 0;       // ticks per beat.
long unsigned int TICKS_PER_STEP = 0;       // ticks per step. (a 16th note)

// 110 hz will be the lowest A. (A2)
// C2 will be the lowest note.
const MusicNote Chroma[] = {
    C2, Cs2, D2, Ds2, E2, F2, Fs2, G2, Gs2, A2, As2, B2,
    C3, Cs3, D3, Ds3, E3, F3, Fs3, G3, Gs3, A3, As3, B3,
    C4, Cs4, D4, Ds4, E4, F4, Fs4, G4, Gs4, A4, As4, B4,
    C5, Cs5, D5, Ds5, E5, F5, Fs5, G5, Gs5, A5, As5, B5};

unsigned int timer0_count = 0;// how many times timer0 has been triggered since last note change
unsigned int timer0_next = 0;
unsigned int timer0_offset = 0; // offset to trigger next note

unsigned int ticks_next; // equivalent to TA0CCR0?
unsigned long ticks_elapsed = 0; // elapsed time since beginning of a note

/* effects */
char effectreg; // effect register
// 11xx xx00
// 11: effect number of voice 1
// 00: effect number of voice 0;

int chord_table_idx = 0;
const char chord_table[12][4] =
{
    {0,0,0,0},
    {0,7,0,7},
    {0,0,7,7},
    {0,0,12,12},
    {0,4,4,7},
    {0,3,3,7},
    {0,0,4,4},
    {0,0,3,3},
    {0,4,0,4},
    {0,3,0,3},
    {0,4,7,11},
    {0,3,7,10}
};
unsigned int chord_index = 0;
unsigned int chord_count = 0;
unsigned int chord_next = 7;

////////////////////////////////////////////////////////////////////////////////

// hz_to_clock
// converts an integer frequency value to the value used to set a
// timer's capture control register
unsigned int hz_to_clock(unsigned int hz);

// make Slice
// encodes passed slice data and returns
// parameters: note in hz, octave multiplier, pulse width, effect, effect 2
Slice slice_make(MusicNote note, int pulsew, int effect, int eparam);

// set current note
// sets the current note a voice including pulse width
void play_note(unsigned int hz, int width, int voice);

// returns current slice
Slice* slice_current();

// processes a slice.
void slice_play(Slice readme);

// current note
// returns the note of the passed slice
MusicNote slice_get_hz(Slice readme);

int slice_get_chroma(Slice readme);

// advances to the next slice in the block
void slice_advance();

// returns the current effect on the passed voice
int effect_flag_get(unsigned int voice);

// sets the effect of a voice
void effect_flag_set(unsigned int voice, unsigned int effect);

void DEBUG_load_block();

#endif
