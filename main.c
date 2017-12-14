#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>


//Function  Declarations

void outchar(unsigned char x);
void tdisp();
void shiftout(char x);
void lcdwait(void);
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
int scan(void);
unsigned char inchar(void);

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
int pushbuttonsarefun = 0;
int testing1 = 0;

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
  SCIBDL =  0x9c; //24,000,000 / 16 / 156 = 9600 (approx)  
  SCICR1 =  0x00; //$9C = 156
  SCICR2 =  0x0C; //initialize SCI for program-driven operation
  DDRB   =  0x10; //set PB4 for output mode
  PORTB  =  0x10; //assert DTR pin on COM port
                  
/* Add additional port pin initializations here */

	  DDRAD = 0; 		//program port AD for input mode
    ATDDIEN = 0xc7; //program PAD7 and PAD6 pins as digital inputs
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

  // Sams inits
    PTT_PTT4 = 0;
    PTT_PTT5 = 0;
    
  
    scan();
    
    
   

}

int scan() {
    int unlock = 0; 
    while (PTAD_PTAD1 == 0) {
     PTT_PTT4 = 0;
     PTT_PTT5 = 1; 
    }
    PTT_PTT4 = 0;
    PTT_PTT5 = 0;
    
    while (PTAD_PTAD2 == 0) {    
    }
    return PTAD_PTAD1;
}


void unlock(){ //Counterclockwise
  PWMDTY0 = 50;
  PWMSCLA = 0x40;
}

void lock() { //clockwise
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
  /*
    if(is_press_finger()) {   	
    	clearlcd();
     	chgline(LINE1);
     	pmsglcd("Examining...");	
       capture_finger(0);
       id = identify1_N();
     	if (id <20) {
     		chgline(LINE2);
     		pmsglcd("Print Found!");
     		leftpb = 1;
     	}
     	else {
     		chgline(LINE2);
     		pmsglcd("Print not Found");
     		rghtpb = 0;
     	}
    } */
    if (LockFlag == 1) {
      chgline(LINE1);
      pmsglcd("  Touch Screen  ");
      chgline(LINE2);
      pmsglcd(" To Gain Access ");
    }
    if(LockFlag == 1) {
        DisableInterrupts;
        if (leftpb == 1) { //If the print has been accepted         
          clearlcd();
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

unsigned char inchar(void) {
        while (!(SCISR1 & 0x20));
    return SCIDRL;
}

void outchar(unsigned char x) {
    while (!(SCISR1 & 0x80));
    SCIDRL = x;
}

