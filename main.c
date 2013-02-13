#include <msp430g2553.h>


void plays(int val, char notes[], int attack_velocity, int release_velocity);
volatile int flag0 = 0x00;
volatile int flag1 = 0x00;
volatile int flag2 = 0x00;
volatile int flag3 = 0x00;
volatile int flag4 = 0x00;
volatile int flag5 = 0x00;
volatile int flag6 = 0x00;
volatile int flag7 = 0x00;

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
void resetMIDI();

//banks
volatile int basic = 0x00;  			//instruments 1-128; notes 30-90
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

	P1DIR |= BIT0;						//tie to MIDI reset
	P1OUT |= 0x00;

	BCSCTL1 = CALBC1_1MHZ;				// Set range
	DCOCTL = CALDCO_1MHZ;				// SMCLK = DCO = 1MHz

	UCA0CTL1 |= UCSWRST; 				// USCI in reset during configuration
	UCA0CTL1 |= UCSSEL_2;				// SMCLK
	UCA0BR0 = 32;						// 1MHz 9600
	UCA0BR1 = 0; 						// 1MHz 9600
	UCA0MCTL = UCBRS0; 					// Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST; 				// Clear USCI software reset



	setVolume(0, 127);
	instrumentSelect(basic, 1);

	while(1){

	for(columnNumber=0;columnNumber<3;columnNumber++){	//step through columns
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

void plays(int val, char notes[], int attack_velocity, int release_velocity)
{
	if(0x00==(val&0x80)){
	noteOn(0, notes[0], attack_velocity);
	flag0=1;
	}
	if(0x00==(val&0x40)){
	noteOn(1, notes[1], attack_velocity);
	flag1=1;}
	if(0x00==(val&0x20)){
	noteOn(2, notes[2], attack_velocity);
	flag2=1;}
	if(0x00==(val&0x10)){
	noteOn(3, notes[3], attack_velocity);
	flag3=1;}
	if(0x00==(val&0x08)){
	noteOn(4, notes[4], attack_velocity);
	flag4=1;}
	if(0x00==(val&0x04)){
	noteOn(5, notes[5], attack_velocity);
	flag5=1;}
	if(0x00==(val&0x02)){
	noteOn(6, notes[6], attack_velocity);
	flag6=1;}
	if(0x00==(val&0x01)){
	noteOn(7, notes[7], attack_velocity);
	flag7=1;}
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
//steps through column and plays notes corresponding to the switch triggered
//NOTE: Look up and check order
void processColumn(int val, char notes[], int attack_velocity, int release_velocity)
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
	P1OUT |= 0x00;
	_delay_cycles(100);
	P1OUT |= 0x01;
	_delay_cycles(100);
}


