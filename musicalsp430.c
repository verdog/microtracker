/*
 * musicalsp430.c
 * various music related functions and data structures for microtracker
 * Josh Chandler
*/

#ifndef MUSICALSP430_C
#define MUSICALSP430_C

#include <msp430.h>
#include "musicalsp430.h"

// hz_to_clock
// converts an integer frequency value to the value used to set a
// timer's capture control register
unsigned int hz_to_clock(unsigned int hz)
{
    // the clock is assumed to be running at 1mhz
    return 1000000/hz/8 + 1;
}

// make Slice
// encodes passed slice data and returns
// parameters: note in hz, octave multiplier, pulse width, effect, effect 2
Slice slice_make(MusicNote note, int pulsew, int effect, int eparam)
{
    int i = 0;
    Slice result = 0;

    // highest 4 bits are the note
    while (Chroma[i] != note && i < 48) i++;
    result |= (i%12) << 12;

    // next 2 bits are the octave
    // set the octave
    result |= ((i/12)%4) << 10;

    // next 2 are pulse width
    if (pulsew < 0 || pulsew > 3) pulsew = 0; // only valid values are 0-3
    result |= pulsew << 8;

    // next 2 are effect
    if (effect < 0 || effect > 3) effect = 0; // only valid values are 0-3
    result |= effect << 6;

    // remaining are effect parameters
    if (eparam < 0 || eparam > 32) eparam = 0; // only valid values are 0-3
    result |= eparam;

    return result;
}

// set current note
// sets the current note a voice including pulse width
void play_note(unsigned int hz, int width, int voice)
{
    if (voice == 0)
    {
        TA0CCR0 = hz_to_clock(hz);
        TA0CCR1 = TA0CCR0/width + 1;
    }
    else
    {
        TA1CCR0 = hz_to_clock(hz);
        TA1CCR1 = TA1CCR0/width + 1;
    }

    return;
}

// returns current slice
Slice* slice_current()
{
    return &Slice_buff[Slice_index];
}

// processes a slice.
void slice_play(Slice readme)
{
    // the first four bytes of a slice are the note value
    int octave = (0x3 & readme >> 10);
    int note = Chroma[((readme >> 12) + 12*octave)%48];
    int effect = (readme >> 6)&3;

    // determine pulse width
    unsigned char pw;
    switch ((readme >> 8) & 0x3)
    {
        case 0: pw = 2;  break;
        case 1: pw = 4;  break;
        case 2: pw = 8;  break;
        case 3: pw = 16; break;
    }

    if (effect == 3) pw = 1; // kill effect

    // play it
    play_note(note, pw, 0);

    return;
}

// current note
// returns the note of the passed slice
MusicNote slice_get_hz(Slice readme)
{
    return Chroma[((readme >> 12) + 12*(0x3 & readme >> 10))%48];
}

// returns the index in the chroma array where you can find the note in this slice
int slice_get_chroma(Slice readme)
{
    return ((readme >> 12) + 12*(0x3 & readme >> 10))%48;
}

// advances to the next slice in the block
void slice_advance()
{
    unsigned int clock_ticks;
    Slice_index = (Slice_index+1)&(BLOCK_SIZE-1);
    clock_ticks = hz_to_clock(slice_get_hz(*slice_current()));

    chord_index = 0;
    chord_count = 0;

	timer0_count = 0;
    timer0_next = TICKS_PER_STEP/clock_ticks;
    timer0_offset = TICKS_PER_STEP - clock_ticks*timer0_next;
	TA0CCR2 = timer0_offset;
    if (*slice_current() != Slice_buff[(Slice_index-1)&(BLOCK_SIZE-1)])
        slice_play(Slice_buff[Slice_index]);

    return;
}

#endif
