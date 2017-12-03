/*
 * microtracker.c
 * a two (hopefully) channel tracker for the msp430 launchpad
 * Josh Chandler
*/

/* this is the chordeffect branch. */

#include <stdlib.h>
#include <msp430.h>
#include "musicalsp430.c"

int main()
{
    // stop watchdog timer
	WDTCTL  = WDTPW | WDTHOLD;
    // run at 1MHz
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL  = CALDCO_1MHZ;

	// load slices
	DEBUG_load_block_0();
	DEBUG_load_block_1();

	//TICKS_PER_BEAT = 65535 - 1; // 19330
	TICKS_PER_STEP = 17050;

    // set up button
    P1REN = 0;
    P1REN |= BIT3; // turn on resistor
    P1OUT = 0;
    P1OUT |= BIT3; // hold resistor high
    P1IE  |= BIT3; // enable button interrupts
    P1IES |= BIT3; // interrupt on falling edge
    P1IFG &=~BIT3; // turn off interrupt flag

    // set up ports
    P1DIR = BIT6 | BIT0; // 1.6
    P1SEL = BIT6;
    P2DIR = BIT1; // 2.1
    P2SEL = BIT1;

    // set up timers
    // voice 0
    TA0CTL  = TASSEL_2 | MC_1 | ID_3; // voice 0
    TA0CCTL0 = CCIE; // frequency interrupt
    TA0CCTL1 = OUTMOD_7; // PWM reset/set
    // TA0CCTL2 = CCIE; // next note interrupt

    // voice 1
    TA1CCTL0 = CCIE;
    TA1CTL  = TASSEL_2 | MC_1 | ID_3; // voice 1
    TA1CCTL1 = OUTMOD_7; // PWM reset/set

    __bis_SR_register(GIE); // interrupts enabled

    // first notes
	slice_advance();

    for(;;);

    return 0;

}

#pragma vector=PORT1_VECTOR
__interrupt void button()
{
    P1OUT ^= BIT0;

    while (!(P1IN & BIT3)); // Wait until button is released
    __delay_cycles(8000); // Debounce
    P1IFG &= ~BIT3; // Turn off interupt flag
    return;
}

#pragma vector=TIMER0_A0_VECTOR // timer 0 interrupt
__interrupt void timer0_A0()
{
	//timer0_count++

	// increase counter
	ticks_elapsed += TA0CCR0;

	// handle effects
	switch (effect_flag_get(0)) {
		case 0: // chord effect
			chord_count_0++;
			if (chord_count_0 >= chord_next)
			{
				chord_count_0 = 0;
				chord_index_0 = (chord_index_0+1)&3; // +1 mod 4

				// change note for chord effect
				// only trigger a note change if it's different than the previous note
				if (chord_table[chord_table_idx_0][chord_index_0] != chord_table[chord_table_idx_0][(chord_index_0-1)&3])
				{
					play_note(Chroma[ (slice_get_chroma(*slice_current(0)) + chord_table[chord_table_idx_0][chord_index_0])%48 ],
					2, 0);
				}
			}
			break;

		case 1:
			// portamento
			TA0CCR0 = TA0CCR0 + slide_speed_0;
			// 1924 = C1
			if (slide_speed_0 > 0 && TA0CCR0 > 3848) // going down
				TA0CCR0 = 64;
			if (slide_speed_0 < 0 && TA0CCR0 < 64) // going up
			 	TA0CCR0 = 3848;
			TA0CCR1 = TA0CCR0/2;
			break;

		case 2:
			slide_tick_0++;
			slide_tick_0 &= 3;

			// pulse width sweep
			if (slide_tick_0 == 0)
			{
				TA0CCR1 += slide_speed_0;
				if (slide_speed_0 > 0 && TA0CCR1 > TA0CCR0)
				{
					slide_speed_0 *= -1;
					TA0CCR1 = TA0CCR0;
				}
				if (slide_speed_0 < 0 && TA0CCR1 > TA0CCR0)
				{
					slide_speed_0 *= -1;
					TA0CCR1 = 0;
				}
			}
			break;
	}

	// calculate how long next frequency will be
	//ticks_next = hz_to_clock(Chroma[ (slice_get_chroma(*slice_current()) + chord_table[chord_index_0])%48 ]);
	ticks_next = TA0CCR0; // set by play_note above

	// check if next note will be triggered
	if (ticks_elapsed + ticks_next > TICKS_PER_STEP)
	{
		// set up next note trigger
		TA0CCR2 = ((ticks_elapsed + ticks_next) - TICKS_PER_STEP);

		// this is to catch cases where TA0CCR2 is set very small
		// which messes up EVERYTHING ;)

		if (TA0CCR2 < ticks_next/2)
		{
			TA0CCR2 = ticks_next/2;
		}

		TA0CCTL2 = CCIE;
	}

	return;
}

#pragma vector=TIMER0_A1_VECTOR // timer 0 ccr1-2 interupt
__interrupt void timer0_A1()
{
    switch(TA0IV)
    {
        case TA0IV_NONE: break;     // should never happen
        case TA0IV_TACCR1: break;   // should never happen
        case TA0IV_TACCR2: 			// end of note interrupt
            slice_advance();
        break;
    }
	return;
}

#pragma vector=TIMER1_A0_VECTOR // timer 1 interrupts
__interrupt void timer1_A0()
{
	// handle effects
	switch (effect_flag_get(1)) {
		case 0: // chord effect
			chord_count_1++;
			if (chord_count_1 >= chord_next)
			{
				chord_count_1 = 0;
				chord_index_1= (chord_index_1+1)&3; // +1 mod 4

				// change note for chord effect
				// only trigger a note change if it's different than the previous note
				if (chord_table[chord_table_idx_1][chord_index_1] != chord_table[chord_table_idx_1][(chord_index_1-1)&3])
				{
					play_note(Chroma[ (slice_get_chroma(*slice_current(1)) + chord_table[chord_table_idx_1][chord_index_1])%48 ],
					2, 1);
				}
			}
			break;

		case 1:
			// portamento
			TA1CCR0 = TA1CCR0 + slide_speed_1;
			// 1924 = C1
			if (slide_speed_1 > 0 && TA1CCR0 > 3848) // going down
				TA1CCR0 = 64;
			if (slide_speed_1 < 0 && TA1CCR0 < 64) // going up
			 	TA1CCR0 = 3848;
			TA1CCR1 = TA1CCR0/2;
			break;

		case 2:
			slide_tick_1++;
			slide_tick_1 &= 3;

			// pulse width sweep
			if (slide_tick_1 == 0)
			{
				TA1CCR1 += slide_speed_1;
				if (slide_speed_1 > 0 && TA1CCR1 > TA1CCR0)
				{
					slide_speed_1 *= -1;
					TA1CCR1 = TA1CCR0;
				}
				if (slide_speed_1 < 0 && TA1CCR1 > TA1CCR0)
				{
					slide_speed_1 *= -1;
					TA1CCR1 = 0;
				}
			}
			break;
	}
}

void DEBUG_load_block_0()
{
    Slice_buff_0 = malloc(BLOCK_SIZE*sizeof(Slice));

	Slice_buff_0[0]  = slice_make(Fs2,3,0,0);
    Slice_buff_0[1]  = slice_make(Fs2,3,0,0);
    Slice_buff_0[2]  = slice_make(Fs4,3,0,0);
    Slice_buff_0[3]  = slice_make(Fs3,3,0,0);
    Slice_buff_0[4]  = slice_make(B5,0,3,31);
    Slice_buff_0[5]  = slice_make(A2,3,0,0);
    Slice_buff_0[6]  = slice_make(G3,0,3,0);
    Slice_buff_0[7]  = slice_make(A3,3,0,0);

    Slice_buff_0[8]  = slice_make(Fs2,3,0,0);
    Slice_buff_0[9]  = slice_make(0,0,3,0);
    Slice_buff_0[10] = slice_make(A3,3,0,0);
    Slice_buff_0[11] = slice_make(D4,3,0,0);
    Slice_buff_0[12] = slice_make(B5,3,3,0);
    Slice_buff_0[13] = slice_make(0,0,3,0);
    Slice_buff_0[14] = slice_make(E4,3,0,0);
    Slice_buff_0[15] = slice_make(D4,3,0,0);

    Slice_buff_0[16] = slice_make(G2,3,0,0);
    Slice_buff_0[17] = slice_make(A3,3,3,0);
    Slice_buff_0[18] = slice_make(G4,3,0,0);
    Slice_buff_0[19] = slice_make(G3,3,0,0);
    Slice_buff_0[20] = slice_make(C5,3,3,0);
    Slice_buff_0[21] = slice_make(B2,3,0,0);
    Slice_buff_0[22] = slice_make(G4,3,3,0);
    Slice_buff_0[23] = slice_make(B3,3,0,0);

    Slice_buff_0[24] = slice_make(G3,3,3,0);
    Slice_buff_0[25] = slice_make(G3,3,3,0);
    Slice_buff_0[26] = slice_make(G3,3,0,0);
    Slice_buff_0[27] = slice_make(G4,3,0,0);
    Slice_buff_0[28] = slice_make(0,3,3,0);
    Slice_buff_0[29] = slice_make(0,3,3,0);
    Slice_buff_0[30] = slice_make(A4,3,0,0);
    Slice_buff_0[31] = slice_make(G4,3,3,0);

	Slice_buff_0[32] = slice_make(A2,3,0,0);
    Slice_buff_0[33] = slice_make(Fs2,3,3,0);
    Slice_buff_0[34] = slice_make(A4,3,0,0);
    Slice_buff_0[35] = slice_make(A3,3,0,0);
    Slice_buff_0[36] = slice_make(B5,3,3,31);
    Slice_buff_0[37] = slice_make(Cs3,3,0,0);
    Slice_buff_0[38] = slice_make(G3,3,3,0);
    Slice_buff_0[39] = slice_make(A3,3,0,0);

    Slice_buff_0[40] = slice_make(0,3,3,0);
    Slice_buff_0[41] = slice_make(0,3,3,0);
    Slice_buff_0[42] = slice_make(Cs4,3,0,0);
    Slice_buff_0[43] = slice_make(D4,3,3,0);
    Slice_buff_0[44] = slice_make(B5,3,3,0);
    Slice_buff_0[45] = slice_make(0,3,3,0);
    Slice_buff_0[46] = slice_make(Cs5,3,0,0);
    Slice_buff_0[47] = slice_make(Cs4,3,0,0);

    Slice_buff_0[48] = slice_make(Fs2,3,0,0);
    Slice_buff_0[49] = slice_make(Fs2,3,0,0);
    Slice_buff_0[50] = slice_make(Fs4,3,0,0);
    Slice_buff_0[51] = slice_make(Fs3,3,0,0);
    Slice_buff_0[52] = slice_make(B5,3,3,31);
    Slice_buff_0[53] = slice_make(A2,3,0,0);
    Slice_buff_0[54] = slice_make(G3,3,3,0);
    Slice_buff_0[55] = slice_make(Fs3,3,0,0);

    Slice_buff_0[56] = slice_make(Fs2,3,0,0);
    Slice_buff_0[57] = slice_make(0,3,3,0);
    Slice_buff_0[58] = slice_make(D3,3,0,0);
    Slice_buff_0[59] = slice_make(D4,3,3,0);
    Slice_buff_0[60] = slice_make(B5,3,3,0);
    Slice_buff_0[61] = slice_make(0,3,3,0);
    Slice_buff_0[62] = slice_make(Fs4,3,0,0);
    Slice_buff_0[63] = slice_make(Fs3,3,0,0);

    return;
}

void DEBUG_load_block_1()
{
	Slice_buff_1 = malloc(BLOCK_SIZE*sizeof(Slice));

	Slice_buff_1[0]  = slice_make(Fs4,0,0,0);
    Slice_buff_1[1]  = slice_make(0,0,3,0);
    Slice_buff_1[2]  = slice_make(D4,0,0,0);
    Slice_buff_1[3]  = slice_make(0,0,3,0);
    Slice_buff_1[4]  = slice_make(A3,0,0,0);
    Slice_buff_1[5]  = slice_make(0,0,3,0);
    Slice_buff_1[6]  = slice_make(Fs4,0,0,0);
    Slice_buff_1[7]  = slice_make(0,0,3,0);

    Slice_buff_1[8]  = slice_make(D4,0,0,0);
    Slice_buff_1[9]  = slice_make(0,0,3,0);
    Slice_buff_1[10] = slice_make(A3,0,0,0);
    Slice_buff_1[11] = slice_make(0,0,3,0);
    Slice_buff_1[12] = slice_make(D4,0,0,0);
    Slice_buff_1[13] = slice_make(0,0,3,0);
    Slice_buff_1[14] = slice_make(Fs4,0,0,0);
    Slice_buff_1[15] = slice_make(0,0,3,0);

    Slice_buff_1[16] = slice_make(B4,0,0,0);
    Slice_buff_1[17] = slice_make(0,0,3,0);
    Slice_buff_1[18] = slice_make(G4,0,0,0);
    Slice_buff_1[19] = slice_make(0,0,3,0);
    Slice_buff_1[20] = slice_make(D4,0,0,0);
    Slice_buff_1[21] = slice_make(0,0,3,0);
    Slice_buff_1[22] = slice_make(B4,0,0,0);
    Slice_buff_1[23] = slice_make(0,0,3,0);

    Slice_buff_1[24] = slice_make(G4,0,0,0);
    Slice_buff_1[25] = slice_make(0,0,3,0);
    Slice_buff_1[26] = slice_make(D4,0,0,0);
    Slice_buff_1[27] = slice_make(0,0,3,0);
    Slice_buff_1[28] = slice_make(G4,0,0,0);
    Slice_buff_1[29] = slice_make(0,0,3,0);
    Slice_buff_1[30] = slice_make(B4,0,0,0);
    Slice_buff_1[31] = slice_make(0,0,3,0);

	Slice_buff_1[32] = slice_make(Cs5,0,0,0);
    Slice_buff_1[33] = slice_make(0,0,3,0);
    Slice_buff_1[34] = slice_make(A4,0,0,0);
    Slice_buff_1[35] = slice_make(0,0,3,0);
    Slice_buff_1[36] = slice_make(E4,0,0,0);
    Slice_buff_1[37] = slice_make(0,0,3,0);
    Slice_buff_1[38] = slice_make(Cs5,0,0,0);
    Slice_buff_1[39] = slice_make(0,0,3,0);

    Slice_buff_1[40] = slice_make(A4,0,0,0);
    Slice_buff_1[41] = slice_make(0,0,3,0);
    Slice_buff_1[42] = slice_make(E4,0,0,0);
    Slice_buff_1[43] = slice_make(0,0,3,0);
    Slice_buff_1[44] = slice_make(A4,0,0,0);
    Slice_buff_1[45] = slice_make(0,0,3,0);
    Slice_buff_1[46] = slice_make(Cs5,0,0,0);
    Slice_buff_1[47] = slice_make(0,0,3,0);

    Slice_buff_1[48] = slice_make(D5,0,0,0);
    Slice_buff_1[49] = slice_make(0,0,3,0);
    Slice_buff_1[50] = slice_make(A4,0,0,0);
    Slice_buff_1[51] = slice_make(0,0,3,0);
    Slice_buff_1[52] = slice_make(Fs4,0,0,0);
    Slice_buff_1[53] = slice_make(0,0,3,0);
    Slice_buff_1[54] = slice_make(D5,0,0,0);
    Slice_buff_1[55] = slice_make(0,0,3,0);

    Slice_buff_1[56] = slice_make(A4,0,0,0);
    Slice_buff_1[57] = slice_make(0,0,3,0);
    Slice_buff_1[58] = slice_make(Fs4,0,0,0);
    Slice_buff_1[59] = slice_make(0,0,3,0);
    Slice_buff_1[60] = slice_make(E5,0,0,0);
    Slice_buff_1[61] = slice_make(0,0,3,0);
    Slice_buff_1[62] = slice_make(D5,0,0,0);
    Slice_buff_1[63] = slice_make(0,0,3,0);

    return;
}
