#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>
#include "finger.h"

//Function  Declarations
char inchar(void);
void outchar(char x);
void tdisp();
void shiftout(char x);
void lcdwait(void);/*
***********************************************************************
 ECE 362 - Experiment 9 - Fall 2016
***********************************************************************
	 	   			 		  			 		  		
 Completed by: < your name >
               < your class number >
               < your lab division >
               < date completed >


 Academic Honesty Statement:  In entering my name above, I hereby certify
 that I am the individual who created this HC(S)12 source file and that I 
 have not copied the work of any other student (past or present) while 
 completing it. I understand that if I fail to honor this agreement, I will 
 receive a grade of ZERO and be subject to possible disciplinary action.
***********************************************************************

 The objective of this experiment is to implement an analog signal sampling
 and reconstruction application that allows the user to efficiently cycle
 through different input and output sampling frequencies.

 The following design kit resources will be used:

 - left pushbutton (PAD7): cycles through input sampling frequency choices
                           (5000 Hz, 10,000 Hz, and 20,000 Hz)

 - right pushbutton (PAD6): cycles through output sampling frequency choices
                           (23,529 Hz, 47,059 Hz, and 94,118 Hz)

 - LCD: displays current values of input and output sampling frequencies
 - Shift Register: performs SPI -> parallel conversion for LCD interface

***********************************************************************
*/
 
#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>

/* All funtions after main should be initialized here */
char inchar(void);
void outchar(char);
void fdisp(void);
void shiftout(char);
void lcdwait(void);
void send_byte(char);
void send_i(char);
void chgline(char);
void print_c(char);
void pmsglcd(char[]);

/*  Variable declarations */ 	   			 		  			 		       
char leftpb	= 0;  // left pushbutton flag
char rghtpb	= 0;  // right pushbutton flag
char prvlpb	= 0;  // previous pushbutton state
char prvrpb	= 0;
int rticnt = 0;
int timcnt = 0;
int in_mode = 0;
int out_mode = 0;
int c = 0;
unsigned long infreq = 5000;
unsigned long outfreq = 23529;

/* LCD COMMUNICATION BIT MASKS */
#define RS 0x04		// RS pin mask (PTT[2])
#define RW 0x08		// R/W pin mask (PTT[3])
#define LCDCLK 0x10	// LCD EN/CLK pin mask (PTT[4])

/* LCD INSTRUCTION CHARACTERS */
#define LCDON 0x0F	// LCD initialization command
#define LCDCLR 0x01	// LCD clear display command
#define TWOLINE 0x38	// LCD 2-line enable command
#define CURMOV 0xFE	// LCD cursor move instruction
#define LINE1 0x80	// LCD line 1 cursor position
#define LINE2 0xC0	// LCD line 2 cursor position

/*	 	   		
***********************************************************************
 Initializations
***********************************************************************
*/

void  initializations(void) {

/* Set the PLL speed (bus clock = 24 MHz) */
  CLKSEL = CLKSEL & 0x80; //; disengage PLL from system
  PLLCTL = PLLCTL | 0x40; //; turn on PLL
  SYNR = 0x02;            //; set PLL multiplier
  REFDV = 0;              //; set PLL divider
  while (!(CRGFLG & 0x08)){  }
  CLKSEL = CLKSEL | 0x80; //; engage PLL


/* Disable watchdog timer (COPCTL register) */
  COPCTL = 0x40   ; //COP off; RTI and COP stopped in BDM-mode

/* Initialize asynchronous serial port (SCI) for 9600 baud, no interrupts */
  SCIBDH =  0x00; //set baud rate to 9600
  SCIBDL =  0x9C; //24,000,000 / 16 / 156 = 9600 (approx)  
  SCICR1 =  0x00; //$9C = 156
  SCICR2 =  0x0C; //initialize SCI for program-driven operation
  DDRB   =  0x10; //set PB4 for output mode
  PORTB  =  0x10; //assert DTR pin on COM port

         
/* Add additional port pin initializations here */
    DDRT = 0xFF;
    PTT = 0x00;
    DDRM = 0x30;

/* Initialize the SPI to 6 Mbs */
    SPICR1 = 0x50;
    SPICR2 = 0x00;
    SPIBR = 0x01;
    
    
    DDRP = 0xFF;
    MODRR	= 0x01;  //PT0 used as PWM Ch 0 output
    PWME	= 0x01; //enable PWM Ch 0
    PWMPOL	= 0x01; //set active high polarity
    PWMCTL	= 0x00; //no concatenate (8-bit)
    PWMCAE	= 0x00; //left-aligned output mode
	  PWMPER0	= 0xFF; //set maximum 8-bit period
	  PWMDTY0	= 0x00; //initially clear DUTY register
    PWMCLK	= 0x00; //select Clock A for Ch 0
    PWMPRCLK	= 0x01; //set Clock A = 12 MHz (prescaler = 2) rate
	 	PWMSCLA = 0x00;
	 	   			 		  			 		  		
/* Initialize digital I/O port pins */

    DDRAD = 0x00;
    ATDDIEN = 0xC0;

/* Initialize the LCD
     - pull LCDCLK high (idle)
     - pull R/W' low (write state)
     - turn on LCD (LCDON instruction)
     - enable two-line mode (TWOLINE instruction)
     - clear LCD (LCDCLR instruction)
     - wait for 2ms so that the LCD can wake up     
*/ 

    PTT_PTT4 = 1;
    PTT_PTT3 = 0;
    send_i(LCDON);
    send_i(TWOLINE);
    send_i(LCDCLR);
    lcdwait();
    

/* Initialize RTI for 2.048 ms interrupt rate */	

    RTICTL = 0x1F; // establish RTI interrupt rate
    CRGINT = CRGINT | 0x80; // enable RTI interrupts
    ATDCTL2 = 0x80;
    ATDCTL3 = 0x10;
    ATDCTL4 = 0x85;
    ATDCTL5 = 0x10;
    while(!ATDSTAT0_SCF) {
    }
    

/* Initialize TIM Ch 7 (TC7) for periodic interrupts every 1.000 ms
     - enable timer subsystem
     - set channel 7 for output compare
     - set appropriate pre-scale factor and enable counter reset after OC7
     - set up channel 7 to generate 1 ms interrupt rate
     - initially disable TIM Ch 7 interrupts      
*/
    TSCR1 = 0x80; // enable timer subsystem
    TSCR2 = 0x0C; // prescale = 16, TCRE bit set
    TIOS = 0x80; // set timer ch 7 for output compare mode
    TIE = 0x80; // enable timer ch 7 interrupts
    TC7 = 0x150; // generate an interrupt every 10 ms
    rticnt=0;
    timcnt=0;
    outchar('A');
 
	      
}

/*	 		  			 		  		
***********************************************************************
 Main
***********************************************************************
*/

void main(void) {
  	DisableInterrupts;
	initializations(); 		  			 		  		
	EnableInterrupts;
	ATDCTL5 = 0x10;
  
   

 

  for(;;) {
  outchar('A');
   /* write your code here */
  if(leftpb)
    {
      leftpb = 0;
      in_mode+=1;
      
      if(in_mode == 0)
      {
        TC7 = 300;
        infreq = 5000; 
      }
      if(in_mode == 1)
      {
        TC7 = 150;
        infreq = 10000;
      }
      if(in_mode == 2)
      {
        TC7 = 75;
        infreq = 20000;
        in_mode = -1;
      }

      
    }


    if(rghtpb)
    {
      rghtpb = 0;
      out_mode+=1;
      
      
      if(out_mode == 0)
      {
        PWMPRCLK = 1;
        PWMSCLA = 0;
        outfreq = 23529;
      }
      if(out_mode == 1)
      {
        PWMPRCLK = 0;
        PWMSCLA = 0;
        outfreq = 47059; 
      }
      if(out_mode == 2)
      {
        PWMPRCLK = 2;
        PWMSCLA = 32;
        outfreq = 94118; 
        out_mode = -1;
      }
      
    }
    fdisp();
  

 
   
  } /* loop forever */
    
}  /* do not leave main */




/*
***********************************************************************
 RTI interrupt service routine: RTI_ISR

  Initialized for 2.048 ms interrupt rate

  Samples state of pushbuttons (PAD7 = left, PAD6 = right)

  If change in state from "high" to "low" detected, set pushbutton flag
     leftpb (for PAD7 H -> L), rghtpb (for PAD6 H -> L)
     Recall that pushbuttons are momentary contact closures to ground
***********************************************************************
*/

interrupt 7 void RTI_ISR(void)
{
  	// clear RTI interrupt flag
  	CRGFLG = CRGFLG | 0x80; 
  	
  	if(PORTAD0_PTAD7 == 0) { // check left pushbutton
            if(prvlpb == 1) {
                leftpb = 1;
            }
        }
        prvlpb = PORTAD0_PTAD7;
        if(PORTAD0_PTAD6 == 0) { // check right pushbutton
            if(prvrpb == 1) {
                rghtpb = 1;
            }
        }
      prvrpb = PORTAD0_PTAD6;

}

/*
***********************************************************************
  TIM interrupt service routine
  used to initiate ATD samples (on Ch 0 and Ch 1)	 		  			 		  		
***********************************************************************
*/

interrupt 15 void TIM_ISR(void)
{

        // clear TIM CH 7 interrupt flag 
 	TFLG1 = TFLG1 | 0x80; 
 	
 	ATDCTL5 = 0x10;
  while(ATDSTAT0);

 	PWMDTY0 = (ATDDR0H * ATDDR1H) / 256;

}

/*
***********************************************************************
  fdisp: Display "ISF = NNNNN Hz" on the first line of the LCD and display 
         and "OSF = MMMMM Hz" on the second line of the LCD (where NNNNN and
         MMMMM are the input and output sampling frequencies, respectively         
***********************************************************************
*/

void fdisp()
{
    chgline(LINE1);
    pmsglcd("ISF = ");
    print_c((infreq/10000)%10 + 48);
    print_c((infreq/1000)%10 + 48);
    print_c((infreq/100)%10 + 48);
    print_c((infreq/10)%10 + 48);
    print_c((infreq%10) + 48);
    
    pmsglcd(" Hz");
    
    chgline(LINE2);
    pmsglcd("OSF = ");
    print_c((outfreq/10000)%10 + 48);
    print_c((outfreq/1000)%10 + 48);
    print_c((outfreq/100)%10 + 48);
    print_c((outfreq/10)%10 + 48);
    print_c((outfreq%10) + 48);
    
    pmsglcd(" Hz");
 
}

/*
***********************************************************************
  shiftout: Transmits the character x to external shift 
            register using the SPI.  It should shift MSB first.  
             
            MISO = PM[4]
            SCK  = PM[5]
***********************************************************************
*/  
 
void shiftout(char x)

{
 
  // read the SPTEF bit, continue if bit is 1
  // write data to SPI data register
  // wait for 30 cycles for SPI data to shift out
   while(!SPISR_SPTEF) {
    }
    // write data to SPI data register
    SPIDR = x;
    // wait for 30 cycles for SPI data to shift out
    lcdwait();

}

/*
***********************************************************************
  lcdwait: Delay for approx 2 ms
***********************************************************************
*/

void lcdwait()
{
    int i = 0;
    for(i = 0; i <= 5000; i++) {
        
    }
 
}

/*
*********************************************************************** 
  send_byte: writes character x to the LCD
***********************************************************************
*/

void send_byte(char x)
{
     // shift out character
     // pulse LCD clock line low->high->low
     // wait 2 ms for LCD to process data
     shiftout(x);
    // pulse LCD clock line low->high->low
    PTT_PTT4 = 0;
    PTT_PTT4 = 1;
    PTT_PTT4 = 0;
    // wait 2 ms for LCD to process data
    lcdwait();
}

/*
***********************************************************************
  send_i: Sends instruction byte x to LCD  
***********************************************************************
*/

void send_i(char x)
{
        // set the register select line low (instruction data)
        // send byte
         PTT_PTT2 = 0;
         send_byte(x);
}

/*
***********************************************************************
  chgline: Move LCD cursor to position x
  NOTE: Cursor positions are encoded in the LINE1/LINE2 variables
***********************************************************************
*/

void chgline(char x)
{
     send_i(CURMOV);
     send_i(x);

}

/*
***********************************************************************
  print_c: Print (single) character x on LCD            
***********************************************************************
*/
 
void print_c(char x)
{
    PTT_PTT2 = 1;
    send_byte(x);

}

/*
***********************************************************************
  pmsglcd: print character string str[] on LCD
***********************************************************************
*/

void pmsglcd(char str[])
{
    int i = 0;
    while(str[i] != 0) {
     print_c(str[i]); 
     i++;
    }

}


/*
***********************************************************************
 Character I/O Library Routines for 9S12C32 (for debugging only)
***********************************************************************
 Name:         inchar
 Description:  inputs ASCII character from SCI serial port and returns it
 Example:      char ch1 = inchar();
***********************************************************************
*/

char inchar(void) {
  /* receives character from the terminal channel */
        while (!(SCISR1 & 0x20)); /* wait for input */
    return SCIDRL;
}

/*
***********************************************************************
 Name:         outchar
 Description:  outputs ASCII character x to SCI serial port
 Example:      outchar('x');
***********************************************************************
*/

void outchar(char x) {
  /* sends a character to the terminal channel */
    while (!(SCISR1 & 0x80));  /* wait for output buffer empty */
    SCIDRL = x;
}
void send_byte(char x);
void send_i(char x);
void chgline(char x);
void print_c(char x);
void pmsglcd(char[]);
void lcdwait_long(void);
void lcdwait_tenSec(void);
void _countdown(void);
void clearlcd(void);
void unlock(void);
void lock(void);
void stop(void);

//Variable Declarations
int rticnt = 0;
int timcnt = 0;
int count = 0; 
char leftpb	= 0;  // left pushbutton flag
char rghtpb	= 0;  // right pushbutton flag
int time = 0;
int wrongs = 0;
int time2 = 0;
int LockFlag = 1; //1 if the safe is locked, 0 if unlocked
long int cycles = 0;
int testaccept = 0;
int countdown = 0;
int lockout = 9;
char prevpbl	= 0;
char prevpbr  = 0;
int LockDoor = 0;
int prevTest1 = 0;
int prevLockButton = 0;
int ACK = 1;
int Error = 0x00;
int pushbuttonsarefun = 0;
int testing1 = 0;
int change = 0;
//int fuckthisshit =0;

//MACROS
/* ASCII character definitions */
#define CR 0x0D	// ASCII return character   

/* LCD COMMUNICATION BIT MASKS */
#define RS 0x04		// RS pin mask (PTT[2])
#define RW 0x08		// R/W pin mask (PTT[3])
#define LCDCLK 0x10	// LCD EN/CLK pin mask (PTT[4])

/* LCD INSTRUCTION CHARACTERS */
#define LCDON 0x0F	// LCD initialization command
#define LCDCLR 0x01	// LCD clear display command
#define TWOLINE 0x38	// LCD 2-line enable command
#define CURMOV 0xFE	// LCD cursor move instruction
#define LINE1 0x80	// LCD line 1 cursor position
#define LINE2 0xC0	// LCD line 2 cursor position

/* LED BIT MASKS */
#define GREEN 0x20
#define RED 0x40
#define YELLOW 0x80

/*
***********************************************************************
 Initializations
***********************************************************************
*/

void initializations(void) {

// Set the PLL speed (bus clock = 24 MHz) 
  CLKSEL = CLKSEL & 0x80; // disengage PLL from system
  PLLCTL = PLLCTL | 0x40; // turn on PLL
  SYNR = 0x02;            // set PLL multiplier
  REFDV = 0;              // set PLL divider
  while (!(CRGFLG & 0x08)){  }
  CLKSEL = CLKSEL | 0x80; // engage PLL


/* Disable watchdog timer (COPCTL register) */
  COPCTL = 0x40;   //COP off, RTI and COP stopped in BDM-mode

/* Initialize asynchronous serial port (SCI) for 9600 baud, no interrupts */
  SCIBDH =  0x00; //set baud rate to 9600
  SCIBDL =  0x9C; //24,000,000 / 16 / 156 = 9600 (approx)  
  SCICR1 =  0x00; //$9C = 156
  SCICR2 =  0x0C; //initialize SCI for program-driven operation
  DDRB   =  0x10; //set PB4 for output mode
  PORTB  =  0x10; //assert DTR pin on COM port
                  
/* Add additional port pin initializations here */

	  DDRAD = 0; 		//program port AD for input mode
    ATDDIEN = 0xC0; //program PAD7 and PAD6 pins as digital inputs
    DDRM = 0x30;
/* Initialize SPI for baud rate of 6 Mbs */
    SPIBR = 0x01; //x = 001, y = 000 => 0_-_0_-_
    SPICR1 = 0x50; //01_1_ _ _ _ 
    SPICR2 = 0;

/* Initialize digital I/O port pins */

    PTM = 0x08;
    PTM = 0x00;
    DDRT = 0xFF;
    PTT = 0x00;

// Initialize the LCD
    PTT_PTT4 = 0x1; //LCDCLK = 0x1;
    PTT_PTT3 = 0x0; //RW' = 0x0; 
    send_i(LCDON);
    send_i(TWOLINE);
    send_i(LCDCLR);
    lcdwait(); 

/* Initialize RTI for 2.048 ms interrupt rate */	
    RTICTL = 0x27;
    CRGFLG = CRGFLG | 0x80;//Set first bit of CRG Flag
    CRGINT = CRGINT | 0x80;//Set first bit of CRG int
    
    ATDCTL2 = 0x80; //power on the ADC and disable interrupts
    ATDCTL3 = 0x10; //select active background mode
    ATDCTL4 = 0x85; //SELECT sample time = 2 ADC clks and set
                     //prescaler to 4 (2 MHz)
                     
    MODRR = 0x01;
    PWME = 0x01;
    PWMPOL = 0x01;
    PWMCAE = 0x00;
    PWMPER0 = 100;
    PWMDTY0 = 0;
    PWMCLK = 0x01;
    PWMCTL = 0x00;
    PWMPRCLK = 0x01;
    PWMSCLA = 0x1E;

    TFLG1 = TFLG1 | 0x80; //enable timer subsystem
    TSCR1 = 0x80;
    TSCR2 = 0x0C;         //channel 7 for output compare
    TIOS = 0x80;
    TIE = 0x80; //disable interrupts
    TC7 = 15000; //10 ms INTERRUPT RATE??      
}

void lock(){ //Counterclockwise
  PWMDTY0 = 50;
  PWMSCLA = 0x40;
}

void unlock() { //clockwise
  PWMDTY0 = 50;
  PWMSCLA = 0x07;
}

void stop() {
  PWMDTY0 = 0;
}

void clearlcd() 
{
  chgline(LINE1);
  pmsglcd("                ");
  chgline(LINE2);
  pmsglcd("                ");
  chgline(LINE1);
}

void main(void){
  DisableInterrupts;
  initializations(); 		  			 		  		
  EnableInterrupts;
  
  clearlcd();
	
  pmsglcd("   TouchSafe:   ");
  chgline(LINE2);
  pmsglcd(" A RAUL Company ");	  
  lcdwait_long();
  clearlcd();
  pmsglcd("  Safe Is Open  ");  
	
  for(;;) {
    cycles = 70000;
  
    if (LockFlag == 1 && change == 0) {
      chgline(LINE1);
      pmsglcd("  Touch Screen  ");
      chgline(LINE2);
      pmsglcd(" To Gain Access ");
    }
    if(LockFlag == 1) {
        DisableInterrupts;
        clearlcd();
        if (leftpb == 1) { //If the print has been accepted
          leftpb = 0;
	        LockDoor = 0;
          pmsglcd("Print Recognized");
          lcdwait_long();
          unlock();
          while(cycles >= 0){
            cycles = cycles-1;
          }
          stop();
          PTT_PTT1 = 0;
          PTT_PTT0 = 1;
          wrongs = 0;
          clearlcd();
          pmsglcd(" Push Button On ");
          chgline(LINE2);
          pmsglcd(" Safe to Relock ");
          lcdwait_long();        
          clearlcd();
          pmsglcd("  Safe Is Open  ");
          LockFlag = 0;
        }
        /*else { //Print Is NOT Accepted
          clearlcd();
          pmsglcd("  Error: Print  ");
          chgline(LINE2);
          pmsglcd(" Not Recognized ");
          lcdwait_long();
          wrongs += 1; 
          if (wrongs == 3) {  //If you enter 3 consecutive incorrect prints, you are locked out for 9 Seconds
            clearlcd();
            pmsglcd(" Maximum Number ");
            chgline(LINE2);
            pmsglcd("Of Tries Reached");
            lcdwait_long();
            clearlcd();
            pmsglcd("Scanner Disabled");
            chgline(LINE2);
            pmsglcd("   For 9 Secs   ");
            lockout = 9;
            _countdown();
            wrongs = 0;
            clearlcd();
          }
        }*/
        EnableInterrupts;
    }
    if(LockFlag == 0){   
      if(rghtpb == 1) { //Safe is Relocking; Input from Pushbutton
        rghtpb = 0;
        DisableInterrupts;
        clearlcd();
        pmsglcd(" Relocking Safe ");
        lcdwait_long();
        lock();
        while(cycles >= 0){
          cycles=cycles-1;
        }
        stop();
        PTT_PTT1 = 1;
        PTT_PTT0 = 0; 
        chgline(LINE2);
        pmsglcd(" Safe Is Locked ");
        lcdwait_long();
        clearlcd();        
        LockFlag = 1;
        EnableInterrupts;
        change = 0;
      }
    }
  }
}
	  
interrupt 15 void TIM_ISR(void)
{
  TFLG1 = TFLG1 | 0x08;
  
  if (countdown == 1) {
    timcnt = (timcnt + 1) % 100;
    if (timcnt == 0) {
      lockout -= 1;
    }
  }
} 

interrupt 7 void RTI_ISR(void)
{
  	// clear RTI interrupt flag
   CRGFLG = CRGFLG | 0x80; 
   rticnt+=1; 
      
      if(rticnt == 48){
        rticnt = 0;
        
        
      }
      
      
      if(PTAD_PTAD7 != prevpbl){ //if pushbutton L isnt equal to its previous value
        if(prevpbl == 1){ //if the previous value was high
          leftpb = 1; //set pb state back to high 
        }else{
          
          leftpb =0;
        }
        
        
      }
      
      if(PTAD_PTAD6 != prevpbr){ //if pushbutton R isnt equal to its previous value
        if(prevpbr == 1){ // if the previous value was high
          rghtpb = 1; // set pb state back to high
        }else{
          
        rghtpb = 0;
        }
        
      }
      if(PTAD_PTAD0 != prevTest1){ //if pushbutton 0 isnt equal to its previous value
        if(prevTest1 == 1){ //if the previous value was high
          testing1 = 1; //set pb state back to high 
        }else{
          
          testing1 =0;
        }
        
        
      }
      
      if(PTAD_PTAD1 != prevLockButton){ //if pushbutton 1 isnt equal to its previous value
        if(prevLockButton == 1){ // if the previous value was high
          LockDoor = 1; // set pb state back to high
        }else{
          
          LockDoor = 0;
        }
        
      }
    
    prevpbl = PTAD_PTAD7;
    prevpbr = PTAD_PTAD6;
    prevTest1 = PTAD_PTAD0;
    prevLockButton = PTAD_PTAD1;
}
void _countdown() {
  countdown = 1;
  
  EnableInterrupts;
  while (lockout != 0) {
    if (lockout == 8)  {
      chgline(LINE2);
      pmsglcd("   For 8 Secs   ");
    } else if (lockout == 7) {
      chgline(LINE2);
      pmsglcd("   For 7 Secs   ");  
    } else if (lockout == 6) {
      chgline(LINE2);
      pmsglcd("   For 6 Secs   ");     
    } else if (lockout == 5) {
      chgline(LINE2);
      pmsglcd("   For 5 Secs   ");       
    } else if (lockout == 4) {
      chgline(LINE2);
      pmsglcd("   For 4 Secs   ");   
    } else if (lockout == 3) {
      chgline(LINE2);
      pmsglcd("   For 3 Secs   ");
    } else if (lockout == 2) {
      chgline(LINE2);
      pmsglcd("   For 2 Secs   "); 
    } else if (lockout == 1) {
      chgline(LINE2);
      pmsglcd("   For 1 Sec    ");     
    }
  }
  countdown = 0;
  DisableInterrupts;
}

void shiftout(char x)

{
  while (!SPISR_SPTEF) {
  }
  SPIDR = x;
  for (count=0; count<30; count++) {
  }
}  
   
void lcdwait()
{
    int count = 0;
    for (count=0; count<4000; count++) { 
    }
}

void lcdwait_long()
{
  time = 0;
	while (time < 2000) {
	  lcdwait();
	  time += 1;
	}
}

void lcdwait_tenSec()
{
  time2 = 0;
	while (time2 < 10000) {
	  lcdwait();
	  time2 += 1;
	}
}

void send_byte(char x)
{
     shiftout(x);
     PTT_PTT4 = 0;
     PTT_PTT4 = 1;
     PTT_PTT4 = 0;
     lcdwait();
}

void send_i(char x)
{
  PTT_PTT2 = 0;
  send_byte(x);
}

void chgline(char x)
{
  send_i(CURMOV);
  send_i(x);  
}

void print_c(char x)
{
    PTT_PTT2 = 1;
    send_byte(x);
}

void pmsglcd(char str[])
{
    int count = 0;
    
    while (str[count] != 0) {
      print_c(str[count]);
      count += 1;
    }
}

char inchar(void) {
        while (!(SCISR1 & 0x20));
    return SCIDRL;
}

void outchar(char x) {
    while (!(SCISR1 & 0x80));
    SCIDRL = x;
}

unsigned char GetHighByte(unsigned int w)
{
    return (unsigned char)(w>>8)&0x00FF;
}

unsigned char GetLowByte(unsigned int w)
{
    return (unsigned char)w&0x00FF;
}

unsigned int byte_to_word(unsigned char* arr)
{
    unsigned int a = 0;
    int i = 0;
    for(i = 0; i< 2; i++) {
        a += arr[i];
        a = a<<8;
    }
    return a;
}

unsigned int _CalculateChecksumOut(struct command_packet* pack)
{
    unsigned char* Parameter = pack->Parameter;
    unsigned char* command = pack-> command;
    unsigned int w = 0;
    w += COMMAND_START_CODE_1;
    w += COMMAND_START_CODE_2;
    w += COMMAND_DEVICE_ID_1;
    w += COMMAND_DEVICE_ID_2;
    w += Parameter[0];
    w += Parameter[1];
    w += Parameter[2];
    w += Parameter[3];
    w += command[0];
    w += command[1];
    
    return w;
}

unsigned char* GetPacketBytes(struct command_packet* pack)
{
    unsigned char* Parameter = pack->Parameter;
    unsigned char* command = pack-> command;
    static unsigned char packetbytes[12];

    // update command before calculating checksum (important!)
    //unsigned int cmd = byte_to_word(command);
    //command[0] = GetLowByte(cmd);
    //command[1] = GetHighByte(cmd);
    
    unsigned int checksum = _CalculateChecksumOut(pack);
    

    packetbytes[0] = COMMAND_START_CODE_1;
    packetbytes[1] = COMMAND_START_CODE_2;
    packetbytes[2] = COMMAND_DEVICE_ID_1;
    packetbytes[3] = COMMAND_DEVICE_ID_2;
    packetbytes[4] = Parameter[0];
    packetbytes[5] = Parameter[1];
    packetbytes[6] = Parameter[2];
    packetbytes[7] = Parameter[3];
    packetbytes[8] = command[0];
    packetbytes[9] = command[1];
    packetbytes[10] = GetLowByte(checksum);
    packetbytes[11] = GetHighByte(checksum);
                         
    return packetbytes;  
}


// returns the 12 bytes of the generated command packet
// remember to call delete on the returned array







int CheckParsing(unsigned char b, unsigned char propervalue, unsigned char alternatevalue)
{
    int retval = (b != propervalue) && (b != alternatevalue);
    
    return retval;
}
/*
void Response_Packet(unsigned char* buffer, struct response_packet* pack)
{
    unsigned char* ParameterBytes = pack->ParameterBytes;
    unsigned char* ResponseBytes = pack->ResponseBytes;
    unsigned char* RawBytes = pack->RawBytes;
    //CheckParsing(buffer[0], COMMAND_START_CODE_1, COMMAND_START_CODE_1, "COMMAND_START_CODE_1");
    //CheckParsing(buffer[1], COMMAND_START_CODE_2, COMMAND_START_CODE_2, "COMMAND_START_CODE_2");
    //CheckParsing(buffer[2], COMMAND_DEVICE_ID_1, COMMAND_DEVICE_ID_1, "COMMAND_DEVICE_ID_1");
    //CheckParsing(buffer[3], COMMAND_DEVICE_ID_2, COMMAND_DEVICE_ID_2, "COMMAND_DEVICE_ID_2");
    int z = 0;
    z = CheckParsing(buffer[8], 0x30, 0x31);
    if (buffer[8] == 0x30) {
        ACK = 1;
    }
    else {
        ACK = 0;
    }
    z = CheckParsing(buffer[9], 0x00, 0x00);
    
    unsigned int checksum = 0;
    checksum = CalculateChecksumIn(buffer, 10);
    unsigned char checksum_low = GetLowByte(checksum);
    unsigned char checksum_high = GetHighByte(checksum);
    CheckParsing(buffer[10], checksum_low, checksum_low);
    CheckParsing(buffer[11], checksum_high, checksum_high);
    
    //int Error = ParseFromBytes(buffer[5], buffer[4]);
    
    ParameterBytes[0] = buffer[4];
    ParameterBytes[1] = buffer[5];
    ParameterBytes[2] = buffer[6];
    ParameterBytes[3] = buffer[7];
    ResponseBytes[0]=buffer[8];
    ResponseBytes[1]=buffer[9];
    int i = 0;
    for (i=0; i < 12; i++)
    {
        RawBytes[i]=buffer[i];
    }
}*/ 

unsigned int CalculateChecksumIn(unsigned char* buffer, int length)
{
    unsigned int checksum = 0;
    int i = 0;
    for (i=0; i<length; i++)
    {
        checksum +=buffer[i];
    }
    return checksum;
}

void input_cmd(unsigned char cmd, unsigned char* arr ) {
    arr[0] = 0x00;
    arr[1] = cmd;
}

void open() {
    
    //static struct command_packet cp;
    //input_cmd(Open, cp.command);
    //cp.Parameter[0] = 0x00;
    //cp.Parameter[1] = 0x00;
    //cp.Parameter[2] = 0x00;
    //cp.Parameter[3] = 0x00;
    //unsigned char* packetbytes = GetPacketBytes(&cp);
    
    // it appears that the 9s12 does not support structures
    unsigned int w = 0;
    w += COMMAND_START_CODE_1;
    w += COMMAND_START_CODE_2;
    w += COMMAND_DEVICE_ID_1;
    w += COMMAND_DEVICE_ID_2;
    w += 0;
    w += 0;
    w += 0;
    w += 0;
    w += GetLowByte(Open);
    w += GetHighByte(Open);
    
    packetbytes[0] = COMMAND_START_CODE_1;
    packetbytes[1] = COMMAND_START_CODE_2;
    packetbytes[2] = COMMAND_DEVICE_ID_1;
    packetbytes[3] = COMMAND_DEVICE_ID_2;
    packetbytes[4] = 0;
    packetbytes[5] = 0;
    packetbytes[6] = 0;
    packetbytes[7] = 0;
    packetbytes[8] = GetHighByte(Open);
    packetbytes[9] = GetLowByte(Open);
    packetbytes[10] = GetLowByte(w);
    packetbytes[11] = GetHighByte(w);
    SendCommand(packetbytes, 12);
    //unsigned char* rp = GetResponse();
    
    return ;
}

void close()
{
    
 static unsigned char packetbytes[12];
    unsigned int w = 0;
    w += COMMAND_START_CODE_1;
    w += COMMAND_START_CODE_2;
    w += COMMAND_DEVICE_ID_1;
    w += COMMAND_DEVICE_ID_2;
    w += 0;
    w += 0;
    w += 0;
    w += 0;
    w += GetLowByte(Close);
    w += GetHighByte(Close);
    
    packetbytes[0] = COMMAND_START_CODE_1;
    packetbytes[1] = COMMAND_START_CODE_2;
    packetbytes[2] = COMMAND_DEVICE_ID_1;
    packetbytes[3] = COMMAND_DEVICE_ID_2;
    packetbytes[4] = 0;
    packetbytes[5] = 0;
    packetbytes[6] = 0;
    packetbytes[7] = 0;
    packetbytes[8] = GetHighByte(Close);
    packetbytes[9] = GetLowByte(Close);
    packetbytes[10] = GetLowByte(w);
    packetbytes[11] = GetHighByte(w);
    SendCommand(packetbytes, 12);
    SendCommand(packetbytes, 12);
    //struct response_packet* rp = GetResponse();      
    
}

void ParameterFromInt(int i, unsigned char* Parameter)
{
    Parameter[0] = (i & 0x000000ff);
    Parameter[1] = (i & 0x0000ff00) >> 8;
    Parameter[2] = (i & 0x00ff0000) >> 16;
    Parameter[3] = (i & 0xff000000) >> 24;
}

void SendCommand(byte cmd[], int length)
{
    int i = 0;
    for(i = 0; i< length; i++) {
       outchar(cmd[i]);
    }
}

unsigned char* GetResponse() {
	  static unsigned char rp[8];
    unsigned char packet[12];
    unsigned char byte = 0x00;
    
    int done = 0;
    
    while (done == 0) {
        while (!(SCISR1 & 0x20));
        byte = SCIDRL;
        if(byte == COMMAND_START_CODE_1)
            done = 1;
        
    }
    packet[0] = byte;
    int x = 1;
    for (x = 1; x <12; x++) {
        while (!(SCISR1 & 0x80));
        packet[x] = SCIDRL;
        
    }
    rp[0] = packet[4];
    rp[1] = packet[5];
    rp[2] = packet[6];
    rp[3] = packet[7];
    rp[4]=  packet[8];
    rp[5]=  packet[9];
    
    
	return rp;
}
  
/*

 

int is_press_finger()
{
    
    unsigned int w = 0;
    w += COMMAND_START_CODE_1;
    w += COMMAND_START_CODE_2;
    w += COMMAND_DEVICE_ID_1;
    w += COMMAND_DEVICE_ID_2;
    w += 0;
    w += 0;
    w += 0;
    w += 0;
    w += GetLowByte(IsPressFinger);
    w += GetHighByte(IsPressFinger);
    
    packetbytes[0] = COMMAND_START_CODE_1;
    packetbytes[1] = COMMAND_START_CODE_2;
    packetbytes[2] = COMMAND_DEVICE_ID_1;
    packetbytes[3] = COMMAND_DEVICE_ID_2;
    packetbytes[4] = 0;
    packetbytes[5] = 0;
    packetbytes[6] = 0;
    packetbytes[7] = 0;
    packetbytes[8] = GetHighByte(IsPressFinger);
    packetbytes[9] = GetLowByte(IsPressFinger);
    packetbytes[10] = GetLowByte(w);
    packetbytes[11] = GetHighByte(w);
    SendCommand(packetbytes, 12);
    //struct response_packet* rp = GetResponse();
    //int retval = 0;
    //int pval = rp->ParameterBytes[0];
    //pval += rp->ParameterBytes[1];
    //pval += rp->ParameterBytes[2];
    //pval += rp->ParameterBytes[3];
    //if (pval == 0)
    //   retval = 1;
    //return retval;  
    return 0;
}

int ChangeBaudRate(unsigned long baud)
{
    if ((baud == 9600) || (baud == 19200) || (baud == 38400) || (baud == 57600) || (baud == 115200))
    {
        
        
        struct command_packet cp;
        input_cmd(Close, cp.command);
        ParameterFromInt(baud, cp.Parameter);
        unsigned char* packetbytes = GetPacketBytes(&cp);
        ;
        SendCommand(packetbytes, 12);
        struct response_packet* rp = GetResponse();
        int retval = ACK;
        if (retval)
        {
            //_serial.end();
            //_serial.begin(baud);
        }
        
        return retval;
    }    
    return 0;
}

int SetLED(int on)
{
    unsigned int on_byte = 0;
    if (on)
    {
        on_byte = 0x01;
    }
    else
    {
        
        on_byte = 0x00;
    }
    unsigned int w = 0;
    w += COMMAND_START_CODE_1;
    w += COMMAND_START_CODE_2;
    w += COMMAND_DEVICE_ID_1;
    w += COMMAND_DEVICE_ID_2;
    w += 0;
    w += 0;
    w += 0;
    w += 0;
    w += GetLowByte(CmosLed);
    w += GetHighByte(CmosLed);
    
    packetbytes[0] = COMMAND_START_CODE_1;
    packetbytes[1] = COMMAND_START_CODE_2;
    packetbytes[2] = COMMAND_DEVICE_ID_1;
    packetbytes[3] = COMMAND_DEVICE_ID_2;
    packetbytes[4] = on_byte;
    packetbytes[5] = 0;
    packetbytes[6] = 0;
    packetbytes[7] = 0;
    packetbytes[8] = GetHighByte(CmosLed);
    packetbytes[9] = GetLowByte(CmosLed);
    packetbytes[10] = GetLowByte(w);
    packetbytes[11] = GetHighByte(w);
    SendCommand(packetbytes, 12);
    //struct response_packet* rp = GetResponse();      
    int retval = 1;
    //if (ACK == 0)
    //    retval = 0;
    return retval; 
}

int IntFromParameter(unsigned char* ParameterBytes) {
    int retval = 0;
    retval = (retval << 8) + ParameterBytes[3];
    retval = (retval << 8) + ParameterBytes[2];
    retval = (retval << 8) + ParameterBytes[1];
    retval = (retval << 8) + ParameterBytes[0];
    return retval;
}

int get_enroll_count()
{
    struct command_packet cp;
    input_cmd(GetEnrollCount, cp.command);
    cp.Parameter[0] = 0x00;
    cp.Parameter[1] = 0x00;
    cp.Parameter[2] = 0x00;
    cp.Parameter[3] = 0x00;
    unsigned char* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    
    int retval = IntFromParameter(rp->ParameterBytes); 
          int retval = 0;
    return retval;
}

int check_enrolled(int id)
{
    
    struct command_packet cp;
    input_cmd(CheckEnrolled, cp.command);
    ParameterFromInt(id, cp.Parameter);
    unsigned char* packetbytes = GetPacketBytes(&cp);
    
    SendCommand(packetbytes, 12);
    
    struct response_packet* rp = GetResponse();   
    int retval = 0;
    retval = ACK;
    return retval;
}

int enroll_start(int id)
{
    struct command_packet cp;
    input_cmd(EnrollStart, cp.command);
    ParameterFromInt(id, cp.Parameter);
    unsigned char* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();      
    int retval = 0;
    if (ACK == 0)
    {
        if (Error == NACK_DB_IS_FULL)
            retval = 1;
        if (Error == NACK_INVALID_POS)
            retval = 2;
        if (Error == NACK_IS_ALREADY_USED)
            retval = 3;
    }
    
    return retval;
}

int enroll1()
{
    struct command_packet cp;
    input_cmd(Enroll1, cp.command);
    unsigned char* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    int retval = IntFromParameter(rp->ParameterBytes);
    if (retval < 20)
        retval = 3;
    else
        retval = 0;
    
    if (ACK == 0)
    {
        if (Error == NACK_ENROLL_FAILED)
            retval = 1;
        if (Error == NACK_BAD_FINGER)
            retval = 2;
    }
    
    if (ACK == 1)
        return 0;
    else
        return retval;
}

int enroll2()
{
    struct command_packet cp;
    input_cmd(Enroll2, cp.command);
    unsigned char* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    int retval = IntFromParameter(rp->ParameterBytes);
    if (retval < 20)
        retval = 3;
    else
        retval = 0;
    
    if (ACK == 0)
    {
        if (Error == NACK_ENROLL_FAILED)
            retval = 1;
        if (Error == NACK_BAD_FINGER)
            retval = 2;
    }
    
    if (ACK == 1)
        return 0;
    else
        return retval;
}

int enroll3()
{
    struct command_packet cp;
    input_cmd(Enroll3, cp.command);
    unsigned char* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    int retval = IntFromParameter(rp->ParameterBytes);
    if (retval < 20)
        retval = 3;
    else
        retval = 0;
    
    if (ACK == 0)
    {
        if (Error == NACK_ENROLL_FAILED)
            retval = 1;
        if (Error == NACK_BAD_FINGER)
            retval = 2;
    }
    
    if (ACK == 1)
        return 0;
    else
        return retval;
}

int identify1_N()
{
    struct command_packet cp;
    input_cmd(Enroll3, cp.command);
    unsigned char* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    int retval = IntFromParameter(rp->ParameterBytes);
    if (retval > 20)
        retval = 20;
    return retval;
}

int capture_finger(int highquality)
{
    struct command_packet cp;
    input_cmd(Enroll3, cp.command);
    
    if (highquality)
    {
        ParameterFromInt(1, cp.Parameter);
    }
    else
    {
        ParameterFromInt(0, cp.Parameter);
    }
    unsigned char* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    int retval = ACK;
    return retval;
    
}

int delete_ID(int id)
{
    struct command_packet cp;
    input_cmd(DeleteID, cp.command);
    ParameterFromInt(id, cp.Parameter);
    unsigned char* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    int retval = ACK;
    return retval;
}                 */
