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
	TICKS_PER_BEAT = 43000 - 1;

    // stop watchdog timer
	WDTCTL  = WDTPW | WDTHOLD;
    // run at 1MHz
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL  = CALDCO_1MHZ;

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
    //TA1CCTL0 = CCIE;
    //TA1CTL  = TASSEL_2 | MC_1 | ID_3; // voice 1
    //TA1CCTL1 = OUTMOD_7; // PWM reset/set

    __bis_SR_register(GIE); // interrupts enabled

    // load slices
    DEBUG_load_block();

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
	//timer0_count++;

	// increase counter
	ticks_elapsed += TA0CCR0;

	// handle effects
	switch (effect_get(0)) {
		case 0: // chord effect

		chord_count++;
		if (chord_count >= chord_next)
		{
			chord_count = 0;
			chord_index = (chord_index+1)&3; // +1 mod 4

			// change note for chord effect
			// only trigger a note change if it's different than the previous note
			if (chord_table[chord_index] != chord_table[(chord_index-1)&3])
			{
				play_note(Chroma[ (slice_get_chroma(*slice_current()) + chord_table[chord_index])%48 ],
				2, 0);
			}
		}

		break;
		case 1:

		// portamento
		chord_index = 5;
		TA0CCR0 -= chord_index;
		TA0CCR1 = TA0CCR0/2;
		TA0R = 0;

		break;
	}

	// calculate how long next frequency will be
	//ticks_next = hz_to_clock(Chroma[ (slice_get_chroma(*slice_current()) + chord_table[chord_index])%48 ]);
	ticks_next = TA0CCR0; // set by play_note above

	// check if next note will be triggered
	if (ticks_elapsed + ticks_next > TICKS_PER_STEP)
	{
		// set up next note trigger
		TA0CCR2 = (ticks_elapsed + ticks_next) - TICKS_PER_STEP;
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
__interrupt void timer1()
{

}

void DEBUG_load_block()
{
    Slice_buff = malloc(BLOCK_SIZE*sizeof(Slice));

	Slice_buff[0]  = slice_make(E2,0,1,0);
    Slice_buff[1]  = slice_make(E2,0,1,0);
    Slice_buff[2]  = slice_make(E2,0,1,0);
    Slice_buff[3]  = slice_make(E2,0,1,0);
    Slice_buff[4]  = slice_make(0,0,3,0);
    Slice_buff[5]  = slice_make(0,0,3,0);
    Slice_buff[6]  = slice_make(E3,1,0,0);
    Slice_buff[7]  = slice_make(E3,1,0,0);

    Slice_buff[8]  = slice_make(0,0,3,0);
    Slice_buff[9]  = slice_make(0,0,3,0);
    Slice_buff[10] = slice_make(E2,0,0,0);
    Slice_buff[11] = slice_make(E2,0,0,0);
    Slice_buff[12] = slice_make(E2,0,0,0);
    Slice_buff[13] = slice_make(E2,0,0,0);
    Slice_buff[14] = slice_make(G2,0,0,0);
    Slice_buff[15] = slice_make(G2,0,0,0);

    Slice_buff[16] = slice_make(A2,0,0,0);
    Slice_buff[17] = slice_make(A2,0,0,0);
    Slice_buff[18] = slice_make(0,0,3,0);
    Slice_buff[19] = slice_make(0,0,3,0);
    Slice_buff[20] = slice_make(A2,0,0,0);
    Slice_buff[21] = slice_make(A2,0,0,0);
    Slice_buff[22] = slice_make(0,0,3,0);
    Slice_buff[23] = slice_make(0,0,3,0);

    Slice_buff[24] = slice_make(A3,1,0,0);
    Slice_buff[25] = slice_make(A3,1,0,0);
    Slice_buff[26] = slice_make(A3,0,0,0);
    Slice_buff[27] = slice_make(A3,0,0,0);
    Slice_buff[28] = slice_make(As2,0,0,0);
    Slice_buff[29] = slice_make(As2,0,0,0);
    Slice_buff[30] = slice_make(0,0,3,0);
    Slice_buff[31] = slice_make(0,0,3,0);

	Slice_buff[32] = slice_make(B2,0,0,0);
    Slice_buff[33] = slice_make(B2,0,0,0);
    Slice_buff[34] = slice_make(B2,0,0,0);
    Slice_buff[35] = slice_make(B2,0,0,0);
    Slice_buff[36] = slice_make(B3,1,0,0);
    Slice_buff[37] = slice_make(B3,1,0,0);
    Slice_buff[38] = slice_make(0,0,3,0);
    Slice_buff[39] = slice_make(0,0,3,0);

    Slice_buff[40] = slice_make(A2,0,0,0);
    Slice_buff[41] = slice_make(A2,0,0,0);
    Slice_buff[42] = slice_make(A2,0,0,0);
    Slice_buff[43] = slice_make(A2,0,0,0);
    Slice_buff[44] = slice_make(A3,1,0,0);
    Slice_buff[45] = slice_make(A3,1,0,0);
    Slice_buff[46] = slice_make(E2,0,0,0);
    Slice_buff[47] = slice_make(E2,0,0,0);

    Slice_buff[48] = slice_make(E2,0,0,0);
    Slice_buff[49] = slice_make(0,0,3,0);
    Slice_buff[50] = slice_make(E2,0,0,0);
    Slice_buff[51] = slice_make(E2,0,0,0);
    Slice_buff[52] = slice_make(E3,1,0,0);
    Slice_buff[53] = slice_make(E3,1,0,0);
    Slice_buff[54] = slice_make(E2,0,0,0);
    Slice_buff[55] = slice_make(E2,0,0,0);

    Slice_buff[56] = slice_make(E3,1,0,0);
    Slice_buff[57] = slice_make(E3,1,0,0);
    Slice_buff[58] = slice_make(D3,0,0,0);
    Slice_buff[59] = slice_make(D3,0,0,0);
    Slice_buff[60] = slice_make(B2,0,0,0);
    Slice_buff[61] = slice_make(B2,0,0,0);
    Slice_buff[62] = slice_make(G2,0,0,0);
    Slice_buff[63] = slice_make(G2,0,0,0);

    return;
}
