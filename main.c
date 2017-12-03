#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>




char leftpb	= 0;  // left pushbutton flag
char rghtpb	= 0;  // right pushbutton flag
char prevpb	= 0;  // previous pushbutton state
int rticnt = 0;
char prevpbl	= 0;
char prevpbr  = 0;
long int cycles =0;
int LockFlag =1; // 1 if locked 0 if unlocked
int testing1, LockDoor = 0;
int prevTest1, prevLockButton = 0;

int pushbuttonsarefun = 0;
//int fuckthisshit =0;


void initializations(void){
      DDRT = 0xFF;
      DDRM = 0x30;
      /* Disable watchdog timer (COPCTL register) */
      COPCTL = 0x40   ; //COP off; RTI and COP stopped in BDM-mode
      /* Add additional port pin initializations here */
     DDRAD = 0; 
     ATDDIEN = 0xC3; //program PAD7 and PAD6 pins as digital inputs
     /* Initialize RTI for 2.048 ms interrupt rate */	
     CRGINT =  0x80; 
     RTICTL =  0x27;
     ATDCTL2 = 0x80; //power on the ADC and disable interrupts
     ATDCTL3 = 0x10; //select active background mode
     ATDCTL4 = 0x85; //SELECT sample time = 2 ADC clks and set
                     //prescaler to 4 (2 MHz)

      MODRR = 0x01;
      PWME = 0x01;
      PWMPOL = 0x01;
      PWMCAE = 0x00;
      PWMPER0 = 100;
      PWMDTY0 = 50;
      PWMCLK = 0x01;
      PWMCTL = 0x00;
      PWMPRCLK = 0x01;
      PWMSCLA = 0x06;
      
}


void main(void){
  DisableInterrupts;
  initializations(); 		  			 		  		
  EnableInterrupts;
  
  //steady = PWMSCLA=0x06;
  
  for(;;){
  cycles = 35000;
  
  if(testing1 == 1){
     testing1 = 0;
     pushbuttonsarefun = 1;
     PTT_PTT1 = 0;
     PTT_PTT0 = 0;
     
  }
  if(LockFlag != 1){
    
  if(LockDoor == 1){
    LockDoor = 0;
    PWMSCLA = 0x0E;
    while(cycles >= 0){
      cycles = cycles-1;
    }
    PWMSCLA = 0x06;
    LockFlag = 1;
  }
  }
    
  if(LockFlag != 1){
    
  if(leftpb == 1){
    leftpb =0;
    
    PWMSCLA = 0x0E;
    while(cycles >= 0){
      cycles = cycles-1;
    }
    PWMSCLA = 0x06;
    LockFlag = 1;
    PTT_PTT1 =0;
    PTT_PTT0 =1;
  }
  
  }
  if(LockFlag != 0){
    
    
  if(rghtpb == 1){
    rghtpb = 0;
    PWMSCLA = 0x04;
    while(cycles >= 0){
      cycles=cycles-1;
    }
    PWMSCLA = 0x06;
    LockFlag = 0;
    PTT_PTT1 = 1;
    PTT_PTT0 = 0;
        
  }
  
  }
  }
}


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






