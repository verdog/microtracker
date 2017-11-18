/*
 * musicalsp430.c
 * various music related functions and data structures for microtracker
 * Josh Chandler
*/

#ifndef MUSICALSP430_C
#define MUSICALSP430_C

#define BLOCK_SIZE 64

typedef unsigned int Slice;

enum NoteHertz // 12 * 4 = 48 notes
{
    C2  = 65,
    Cs2 = 69,
    D2  = 73,
    Ds2 = 78,
    E2  = 82,
    F2  = 87,
    Fs2 = 93,
    G2  = 98,
    Gs2 = 104,
    A2  = 110,
    As2 = 117,
    B2  = 123,
    C3  = 131,
    Cs3 = 139,
    D3  = 147,
    Ds3 = 156,
    E3  = 165,
    F3  = 175,
    Fs3 = 185,
    G3  = 196,
    Gs3 = 208,
    A3  = 220,
    As3 = 233,
    B3  = 247,
    C4  = 262,
    Cs4 = 277,
    D4  = 294,
    Ds4 = 311,
    E4  = 330,
    F4  = 349,
    Fs4 = 370,
    G4  = 392,
    Gs4 = 415,
    A4  = 440,
    As4 = 466,
    B4  = 494,
    C5  = 523,
    Cs5 = 554,
    D5  = 587,
    Ds5 = 622,
    E5  = 659,
    F5  = 698,
    Fs5 = 740,
    G5  = 784,
    Gs5 = 831,
    A5  = 880,
    As5 = 932,
    B5  = 988
};
typedef enum NoteHertz MusicNote;

// 110 hz will be the lowest A. (A2)
// C2 will be the lowest note.
const MusicNote Chroma[] = {
    C2, Cs2, D2, Ds2, E2, F2, Fs2, G2, Gs2, A2, As2, B2,
    C3, Cs3, D3, Ds3, E3, F3, Fs3, G3, Gs3, A3, As3, B3,
    C4, Cs4, D4, Ds4, E4, F4, Fs4, G4, Gs4, A4, As4, B4,
    C5, Cs5, D5, Ds5, E5, F5, Fs5, G5, Gs5, A5, As5, B5};

// hz_to_clock
// converts an integer frequency value to the value used to set a
// timer's capture control register
unsigned int hz_to_clock(unsigned int hz)
{
    // the clock is assumed to be running at 1mhz
    return 1000000/hz/8 + 1;
}

Slice *Slice_buff; // global slice buffer. 32 slices.
int Slice_index = -1;   // Slice_index in Slice array

long unsigned int TICKS_PER_BEAT;       // ticks per beat.
#define TICKS_PER_STEP TICKS_PER_BEAT/4 // ticks per step. (a 16th note)

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

int slice_get_chroma(Slice readme)
{
    return ((readme >> 12) + 12*(0x3 & readme >> 10))%48;
}

unsigned int timer0_count = 0;// how many times timer0 has been triggered since last note change
unsigned int timer0_next = 0;
unsigned int timer0_offset = 0; // offset to trigger next note

int chord_table[4] = {0, 0, 12, 12};
unsigned int chord_index = 0;
unsigned int chord_count = 0;
unsigned int chord_next = 2;

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

void DEBUG_load_block();

#endif
