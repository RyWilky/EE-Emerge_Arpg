#include <msp430g2553.h>

/* 2/18/2013 Notes
 * included functions to sample buttons and the ADC to determine
 * instrument being played, volume, and speed.
 * Set volume and set instrument do not work atm
 */


void plays(int val, char notes[], int attack_velocity, int release_velocity);

volatile int dummy = 0x00;
volatile int j = 0;
volatile int columnNumber = 0;
volatile int test[] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};
volatile int lights[]= {0x01, 0x02, 0x04, 0x08, 0x10, 0x20,0x40,0x80};
//volatile int test[]= {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
char notes[]= {70, 71, 72, 73, 74, 75, 76, 77};
volatile int column[8];
volatile int testPlay = 1;
volatile int connect = 0;
void set(int address, int bits);
int get(int address);

void processColumn(int val, char notes[], int attack_velocity, int release_velocity);
void writeMIDI(int command, int data1, int data2);
void instrumentSelect(int bank, int instrument);
void play(int channel, int note, int attack_velocity, int release_velocity); //half velocity = D69
void noteOn(int channel, int note, int attack_velocity);
void noteOff(int channel, int note, int release_velocity);
void setVolume(int channel, int volume);
void checkInstrument(char mode1, char mode2);
void checkVol_and_Speed();
void resetMIDI();
int get_ADC(int pin);

volatile int volSet = 0x00;
volatile int speedSet = 0x00;
volatile char pin = 0x00;
volatile char mode2=0x00;
volatile char mode1=0x00;

//banks
volatile int basic = 0x00;				//instruments 1-128; notes 30-90
volatile int percussion = 0x78;			//instruments 30; notes 27-87
volatile int melodic = 0x79;			//instruments 27-86; notes 30-90


void main(void)
{

	//Clock Config
	WDTCTL = WDTPW + WDTHOLD; 			// Stop WDT
	BCSCTL1 = CALBC1_1MHZ; 				// Set DCO = smclk = 1 MHz
	DCOCTL = CALDCO_1MHZ;

	//UART Config & I2C Config
	P1SEL = BIT1 + BIT2 + BIT6 + BIT7; 	// P1.1 = RXD, P1.2=TXD, I2C pins to USCI_B0
	P1SEL2 = BIT1 + BIT2 + BIT6 + BIT7; // P1.1 = RXD, P1.2=TXD, I2C pins to USCI_B0

	P1DIR |= BIT3;						//tie to MIDI reset
	P1OUT |= BIT3;						//Turn shield on

	BCSCTL1 = CALBC1_1MHZ;				// Set range
	DCOCTL = CALDCO_1MHZ;				// SMCLK = DCO = 1MHz

	UCA0CTL1 |= UCSWRST; 				// USCI in reset during configuration
	UCA0CTL1 |= UCSSEL_2;				// SMCLK
	UCA0BR0 = 32;						// 1MHz 9600
	UCA0BR1 = 0; 						// 1MHz 9600
	UCA0MCTL = UCBRS0; 					// Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST; 				// Clear USCI software reset

	P2DIR |= BIT1 + BIT2;	//enable indicator LEDs for mode flags
	P2OUT = 0x00;			//make sure off
	P1IE |= BIT0; 			// P1.0 interrupt enabled
	P1IFG &= ~(BIT0); 		// P1.0 IFG cleared
	P2IE |= BIT0; 			// P2.0 interrupt enabled
	P2IFG &= ~(BIT0); 		// P2.0 IFG cleared
	__enable_interrupt(); 	// enable all interrupts


	resetMIDI();
	setVolume(0, 127);
	instrumentSelect(percussion, 33);

	while(1){

	for(columnNumber=0;columnNumber<3;columnNumber++){	//step through columns
		checkVol_and_Speed();
		checkInstrument(mode1, mode2);
		set(0x20,lights[columnNumber]);					//turn on column LEDs
		_delay_cycles(1000);
		set(0x21, test[columnNumber]);				//setting column to sample
		column[columnNumber] = get(0x22);				//sample column
		plays(column[columnNumber], notes, 69, 69);					//process & play column notes
		_delay_cycles(100);
		set(0x20,0x00);									//turn off LEDs
		_delay_cycles(100000);
	}


}
}


//Samples the Knobs and throws it into volSet or speedSet
//get ADC receives an 10-bit number, setVolume takes a 7 bit number
//vol is pretty easy to set.
//can probably do something with speed
//maybe abstract to different fct
void checkVol_and_Speed()
{
	volSet = get_ADC(4);
	volSet = volSet/8;
	setVolume(0,volSet);
	speedSet = get_ADC(5);
	speedSet = speedSet/8;

}


//supposed to check mode1 and mode2 and switch the instrument
//having problems with instrumentSelect atm
void checkInstrument(char mode1, char mode2)
{
	if(mode1==0 && mode2==0)
		instrumentSelect(percussion, 36);		//bright piano
	else if(mode1==0 && mode2==1)
		instrumentSelect(basic, 24);	//synth strings1
	else if(mode1==1 && mode2==0)
		instrumentSelect(basic, 79);	//ocarina
	else if(mode1==1 && mode2==1)
		instrumentSelect(basic, 1);	//xylophone
}

void plays(int val, char notes[], int attack_velocity, int release_velocity)
{
	if(0x00==(val&0x80)){
	noteOn(0, notes[0], attack_velocity);}
	if(0x00==(val&0x40)){
	noteOn(1, notes[1], attack_velocity);}
	if(0x00==(val&0x20)){
	noteOn(2, notes[2], attack_velocity);}
	if(0x00==(val&0x10)){
	noteOn(3, notes[3], attack_velocity);}
	if(0x00==(val&0x08)){
	noteOn(4, notes[4], attack_velocity);}
	if(0x00==(val&0x04)){
	noteOn(5, notes[5], attack_velocity);}
	if(0x00==(val&0x02)){
	noteOn(6, notes[6], attack_velocity);}
	if(0x00==(val&0x01)){
	noteOn(7, notes[7], attack_velocity);}
	_delay_cycles(500000);
	if(0x00==(val&0x80))
	noteOff(0, notes[0], release_velocity);
	if(0x00==(val&0x40))
	noteOff(1, notes[1], release_velocity);
	if(0x00==(val&0x20))
	noteOff(2, notes[2], release_velocity);
	if(0x00==(val&0x10))
	noteOff(3, notes[3], release_velocity);
	if(0x00==(val&0x08))
	noteOff(4, notes[4], release_velocity);
	if(0x00==(val&0x04))
	noteOff(5, notes[5], release_velocity);
	if(0x00==(val&0x02))
	noteOff(6, notes[6], release_velocity);
	if(0x00==(val&0x01))
	noteOff(7, notes[7], release_velocity);
	_delay_cycles(500000);

}


void instrumentSelect(int bank, int instrument)
{
	writeMIDI(0xB0, 0, bank);
	writeMIDI(0xC0, instrument, 0);
}

void noteOn(int channel, int note, int attack_velocity)
{
	writeMIDI((0x90 |channel), note, attack_velocity);
}

void noteOff(int channel, int note, int release_velocity)
{
	writeMIDI((0x80 | channel), note, release_velocity);
}

void setVolume(int channel, int volume)
{
	if(volume >127){
		volume = 127;
		writeMIDI((0xB0|channel), 0x07, volume);
	}
	if(volume <0){
		volume = 0;
		writeMIDI((0xB0|channel), 0x07, volume);
	}
	else
		writeMIDI((0xB0|channel), 0x07, volume);
}

void resetMIDI()
{
	P1OUT &= ~(BIT3);
	_delay_cycles(10);
	P1OUT |= BIT3;
}

void writeMIDI(int command, int data1, int data2)
{
		while (!(IFG2&UCA0TXIFG)) {;} 		// wait for USCI_A0 TX buffer ready
		UCA0TXBUF = (command);				// send bits 9-2
		while (!(IFG2&UCA0TXIFG)) {;} 		// wait for USCI_A0 TX buffer ready
		UCA0TXBUF = (data1);				// send bits 1-0

		if((command & 0xF0)<=0xB0){			//commands less than 0xBn have 2 data bytes
		while (!(IFG2&UCA0TXIFG)) {;} 		// wait for USCI_A0 TX buffer ready
		UCA0TXBUF = (data2);				// send bits 1-0
		}
}

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
	if(P1IFG&0x1==0x01){
		P2OUT ^=BIT1;
		mode1 ^= (0x01); // mode1 = toggle
		P1IFG &= ~BIT0;} // P1.0 IFG cleared
}

#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{
	if(P2IFG&0x01==0x01){
		P2OUT ^=BIT2;
		mode2 ^= (0x01); // mode0 = toggle
		P2IFG &= ~BIT0;} // P2.0 IFG cleared
}

void set(int address, int bits)
{
		UCB0CTL1 = UCSWRST;						//Hold I2C bus during initiation
		UCB0CTL0 = UCMODE_3 + UCMST + UCSYNC; 	//sets I2C mode, master mode, synchronous mode
		UCB0CTL1 |= UCSSEL_3;					//select SMCLK
		UCB0BR0 = 12;							//slow down SMCLK, divide by 12
		UCB0BR1 = 0;							//1kHz/12 = ~83 Hz
		UCB0I2CSA = address;					//slave address(0101000)
		UCB0CTL1 &= ~UCSWRST;					//enable i2c
		while(UCB0CTL1 & UCTXSTP);				//make sure Stop bit is not set
		UCB0CTL1 |= UCTR + UCTXSTT;				//set transmit, generate start
		while(!(IFG2 & UCB0TXIFG)){;}			//wait for transmission to finish
		UCB0TXBUF = bits;						//set data
		while(!(IFG2 & UCB0TXIFG)){;}			//wait for transmission to finish
		UCB0CTL1 |= UCTXSTP;					//generate stop condition
		while(UCB0CTL1 & UCTXSTP){;}			//wait for transmission to finish
		UCB0CTL1 |= UCSWRST;					//release I2C bus
}

int get(int address)
{
		UCB0CTL1 = UCSWRST;						//Hold I2C bus during initiation
		UCB0CTL0 = UCMODE_3 + UCMST + UCSYNC; 	//sets I2C mode, master mode, synchronous mode
		UCB0CTL1 |= UCSSEL_3;					//select SMCLK
		UCB0BR0 = 12;							//slow down SMCLK, divide by 12
		UCB0BR1 = 0;							//1kHz/12 = ~83 Hz
		UCB0I2CSA = address;					//slave address(0101000)
		UCB0CTL1 &= ~UCSWRST;					//enable i2c
		while(UCB0CTL1 & UCTXSTP);				//make sure Stop bit is not set
		UCB0CTL1 &= ~UCTR;
		UCB0CTL1 |= UCTXSTT;					//receive mode, generate start
		while(!(IFG2 & UCB0RXIFG)){;}			//wait for transmission to finish
		connect = UCB0RXBUF;					//set data
		UCB0CTL1 |= UCTXSTP;					//generate stop condition
		while(UCB0CTL1 & UCTXSTP){;}			//wait for transmission to finish
		UCB0CTL1 |= UCSWRST;					//release I2C bus
		return connect;
}

int get_ADC(int pin)
{
	if(pin==4)
		ADC10CTL1 = INCH_4 + ADC10SSEL_3; 					// Read pin P1.4, use SMCLK
	if(pin==5)
		ADC10CTL1 = INCH_5 + ADC10SSEL_3; 					// Read pin P1.5, use SMCLK
	ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON;	//use Vcc and Vss and R+ and R-, 16 clock ticks per acquisition
														//internal reference on, ADC on, enable ADC
	_delay_cycles(10); 								// Wait for ADC Ref to settle
	ADC10CTL0 |= ENC + ADC10SC; 						// Sampling & conversion start
	_delay_cycles(100);									//acquiring data
	ADC10CTL0 &= ~ENC;									//disable ADC
	ADC10CTL0 &= ~(REFON + ADC10ON);					//turn off ADC and reference
	dummy = ADC10MEM;
	_delay_cycles(10);
	return dummy;
}

