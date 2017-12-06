/*
 * editor.h
 * defines functions for the GUI used to edit songs
 * Josh Chandler
*/

#ifndef EDITOR_H
#define EDITOR_H

#include <libemb/serial/serial.h>
#include <libemb/conio/conio.h>

int EDIT_MODE = 0;
/* modes:
   0 = navigate
   1 = edit value
*/

int EDIT_PRINTBOOL = 1;
unsigned int EDIT_PRINTRANGE = 0;
int EDIT_ROW = 0;
int EDIT_COL = 0;
const unsigned int EDIT_ROW_TOTAL = BLOCK_SIZE;
const unsigned int EDIT_COL_TOTAL = 8;

const char chroma_str[60][4] =
{
        "C2 \0", "Cs2\0", "D2 \0", "Ds2\0", "E2 \0", "F2 \0",
        "Fs2\0", "G2 \0", "Gs2\0", "A2 \0", "As2\0", "B2 \0",
        "C3 \0", "Cs3\0", "D3 \0", "Ds3\0", "E3 \0", "F3 \0",
        "Fs3\0", "G3 \0", "Gs3\0", "A3 \0", "As3\0", "B3 \0",
        "C4 \0", "Cs4\0", "D4 \0", "Ds4\0", "E4 \0", "F4 \0",
        "Fs4\0", "G4 \0", "Gs4\0", "A4 \0", "As4\0", "B4 \0",
        "C5 \0", "Cs5\0", "D5 \0", "Ds5\0", "E5 \0", "F5 \0",
        "Fs5\0", "G5 \0", "Gs5\0", "A5 \0", "As5\0", "B5 \0",
        "C6 \0", "Cs6\0", "D6 \0", "Ds6\0", "E6 \0", "F6 \0",
        "Fs6\0", "G6 \0", "Gs6\0", "A6 \0", "As6\0", "B6 \0"
};

// displayes all slices in buffer
void display_slices();

// display_slices_precise
void display_slices_p(unsigned int start, unsigned int end);

// moves cursor across the editing area.
// dir is read like a number on the number pad.
// 6 is right, 4 is left, 8 is up, 2 is down.
void cursor_move(char dir);

// hz to char*
// outputs a char string representing the hz of a note
char* slice_to_hzstr(Slice readme, int voice);

int slice_to_pw(Slice readme);
int slice_to_effect(Slice readme);
int slice_to_effectp(Slice readme);

void edit_value();

#endif
