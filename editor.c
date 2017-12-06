/*
 * editor.c
 * defines the function bodies for the GUI used to edit songs
 * Josh Chandler
*/

#ifndef EDITOR_C
#define EDITOR_C

#include "editor.h"


void display_slices()
{
    if (EDIT_PRINTBOOL == 0)
        display_slices_p(
            EDIT_ROW - EDIT_PRINTRANGE,
            EDIT_ROW + EDIT_PRINTRANGE);
    else
        display_slices_p(0, BLOCK_SIZE-1);
    return;
}

void display_slices_p(unsigned int start, unsigned int end)
{
    int i;
    if (start > end) start = 0;
    if (end > BLOCK_SIZE-1) end = BLOCK_SIZE-1;

    char sc = '0'; // "special char", used for nav marker
    for (i=start; i<=end; i++)
    {
        // row number
        if (i < 10) cio_printf("0");
        cio_printf("%u: ", i);

        // pitch0
        if (EDIT_ROW == i && EDIT_COL == 0) sc = '#';
        else sc = ' ';
        cio_printf("%c", sc);
        cio_printf("%s", slice_to_hzstr(Slice_buff_0[i], 0));
        cio_printf("%c ", sc);

        // pw0
        if (EDIT_ROW == i && EDIT_COL == 1) sc = '#';
        else sc = ' ';
        cio_printf("%c", sc);
        cio_printf("w: %i", slice_to_pw(Slice_buff_0[i]));
        cio_printf("%c ", sc);

        // effect0
        if (EDIT_ROW == i && EDIT_COL == 2) sc = '#';
        else sc = ' ';
        cio_printf("%c", sc);
        cio_printf("e: %i", slice_to_effect(Slice_buff_0[i]));
        cio_printf("%c ", sc);

        // effect0p
        if (EDIT_ROW == i && EDIT_COL == 3) sc = '#';
        else sc = ' ';
        cio_printf("%cep:", sc);
        if (slice_to_effectp(Slice_buff_0[i]) < 10) cio_printf("0");
        cio_printf("%i", slice_to_effectp(Slice_buff_0[i]));
        cio_printf("%c ", sc);

        // divider
        cio_printf("||");

        // pitch1
        if (EDIT_ROW == i && EDIT_COL == 4) sc = '#';
        else sc = ' ';
        cio_printf("%c", sc);
        cio_printf("%s", slice_to_hzstr(Slice_buff_1[i], 1));
        cio_printf("%c ", sc);

        // pw1
        if (EDIT_ROW == i && EDIT_COL == 5) sc = '#';
        else sc = ' ';
        cio_printf("%c", sc);
        cio_printf("w: %i", slice_to_pw(Slice_buff_1[i]));
        cio_printf("%c ", sc);

        // effect1
        if (EDIT_ROW == i && EDIT_COL == 6) sc = '#';
        else sc = ' ';
        cio_printf("%c", sc);
        cio_printf("e: %i", slice_to_effect(Slice_buff_1[i]));
        cio_printf("%c ", sc);

        // effect1p
        if (EDIT_ROW == i && EDIT_COL == 7) sc = '#';
        else sc = ' ';
        cio_printf("%cep:", sc);
        if (slice_to_effectp(Slice_buff_1[i]) < 10) cio_printf("0");
        cio_printf("%i", slice_to_effectp(Slice_buff_1[i]));
        cio_printf("%c ", sc);

        cio_printf("\n\r");
    }

    return;
}

void cursor_move(char dir)
{
    switch (dir)
    {
        // cardinal
        case '8':
            EDIT_ROW--;
        break;
        case '6':
            EDIT_COL++;
        break;
        case '4':
            EDIT_COL--;
        break;
        case '2':
            EDIT_ROW++;
        break;

        // quick
        case '9': // top of list
            EDIT_ROW = 0;
        break;
        case '3': // bottom of list
            EDIT_ROW = BLOCK_SIZE-1;
        break;
        case '7': // 1/4 up
            EDIT_ROW -= BLOCK_SIZE/4;
        break;
        case '1': // 1/4 up
            EDIT_ROW += BLOCK_SIZE/4;
        break;

        default: return; // invalid move.
    }

    EDIT_ROW %= EDIT_ROW_TOTAL;
    EDIT_COL %= EDIT_COL_TOTAL;

    return;
}

char* slice_to_hzstr(Slice readme, int voice)
{
    voice &= 1;
    if (voice == 0) return chroma_str[slice_get_chroma(readme)];
    else            return chroma_str[slice_get_chroma(readme) + 12];
}

int slice_to_pw(Slice readme)
{
    return ((readme >> 8) & 3);
}

int slice_to_effect(Slice readme)
{
    return ((readme >> 6) & 3);
}

int slice_to_effectp(Slice readme)
{
    return (readme & 63);
}

void edit_value()
{
    int reqc = 0; // required chars
    int apply = 0;
    int i = 0;
    char entry[4];

    switch (EDIT_COL) {
        case 0: case 4: // pitch
            reqc = 3;
        break;
        case 1: case 2: case 5: case 6: // pw/effect
            reqc = 1;
        break;
        case 3: case 7: // effect param
            reqc = 2;
        break;
        default: reqc = 99; // should never happen
    }

    cio_printf("Please enter %i characters: ", reqc);

    for (i = 0; i < reqc; i++)
    {
        entry[i] = cio_getc(); // get char
        cio_printf("%c", entry[i]); // echo char
    }
    cio_printf("\n\r");

    switch (reqc) {
        case 1:
            if (EDIT_COL == 1) // pw0
            {
                apply = ((int)(entry[0] - 48))&3;
                Slice_buff_0[EDIT_ROW] &= ~(3 << 8); // erase old
                Slice_buff_0[EDIT_ROW] |= (apply << 8); // set new
            }
            else if (EDIT_COL == 2) // effect0p
            {
                apply = ((int)(entry[0] - 48))&3;
                Slice_buff_0[EDIT_ROW] &= ~(3 << 6); // erase old
                Slice_buff_0[EDIT_ROW] |= (apply << 6); // set new
            }
            else if (EDIT_COL == 5) // pw1
            {
                apply = ((int)(entry[0] - 48))&3;
                Slice_buff_1[EDIT_ROW] &= ~(3 << 8); // erase old
                Slice_buff_1[EDIT_ROW] |= (apply << 8); // set new
            }
            else if (EDIT_COL == 6) // effect1
            {
                apply = ((int)(entry[0] - 48))&3;
                Slice_buff_1[EDIT_ROW] &= ~(3 << 6); // erase old
                Slice_buff_1[EDIT_ROW] |= (apply << 6); // set new
            }
        break;
        case 2:
            if (EDIT_COL == 3) // effectp0
            {
                apply = 10 * ((int)(entry[0] - 48));
                apply += ((int)(entry[1] - 48));
                apply &= 63;

                Slice_buff_0[EDIT_ROW] &= ~(63); // erase old
                Slice_buff_0[EDIT_ROW] |= (apply); // set new
            }
            else if (EDIT_COL == 7) // effectp1
            {
                apply = 10 * ((int)(entry[0] - 48));
                apply += ((int)(entry[1] - 48));
                apply &= 63;

                Slice_buff_1[EDIT_ROW] &= ~(63); // erase old
                Slice_buff_1[EDIT_ROW] |= (apply); // set new
            }
        break;
        case 3:
            if (entry[0] < 65 || entry[0] > 71) break; // is it capital A-G?
            if (EDIT_COL == 0) // pitch0
            {
                i = 0;
                while ((entry[0] != chroma_str[i][0] ||
                       entry[1] != chroma_str[i][1] ||
                       entry[2] != chroma_str[i][2]) &&
                       i < 48)
                { i++; } // search for match

                Slice_buff_0[EDIT_ROW] &= ~(63 << 10); // erase old
                Slice_buff_0[EDIT_ROW] |= (i%12) << 12; // set note
                Slice_buff_0[EDIT_ROW] |= ((i/12)%4) << 10; // set octave
                break;
            }
            else if (EDIT_COL == 4) // pitch1
            {
                i = 12;
                while ((entry[0] != chroma_str[i][0] ||
                       entry[1] != chroma_str[i][1] ||
                       entry[2] != chroma_str[i][2]) &&
                       i < 60)
                { i++; } // search for match

                Slice_buff_1[EDIT_ROW] &= ~(63 << 10); // erase old
                Slice_buff_1[EDIT_ROW] |= ((i-12)%12) << 12; // set note
                Slice_buff_1[EDIT_ROW] |= (((i-12)/12)%4) << 10; // set octave
                break;
            }
        break;

        default: break;
    }

    return;
}

#endif
