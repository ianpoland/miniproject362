#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>
#include "finger.h"




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
int ACK = 1;
int Error = 0x00;
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

byte GetHighByte(word w)
{
    return (byte)(w>>8)&0x00FF;
}

byte GetLowByte(word w)
{
    return (byte)w&0x00FF;
}

// returns the 12 bytes of the generated command packet
// remember to call delete on the returned array
word _CalculateChecksumOut(struct command_packet* pack)
{
    byte* Parameter = pack->Parameter;
    byte* command = pack-> command;
    word w = 0;
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


word byte_to_word(byte* arr)
{
    word a = 0;
    int i = 0;
    for(i = 0; i< 2; i++) {
        a += arr[i];
        a = a<<8;
    }
    return a;
}

byte* GetPacketBytes(struct command_packet* pack)
{
    byte* Parameter = pack->Parameter;
    byte* command = pack-> command;
    static byte packetbytes[12];
    
    // update command before calculating checksum (important!)
    word cmd = byte_to_word(command);
    command[0] = GetLowByte(cmd);
    command[1] = GetHighByte(cmd);
    
    word checksum = _CalculateChecksumOut(pack);
    
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


int CheckParsing(byte b, byte propervalue, byte alternatevalue, const char* varname)
{
    int retval = (b != propervalue) && (b != alternatevalue);
    
    return retval;
}

void Response_Packet(byte* buffer, struct response_packet* pack)
{
    byte* ParameterBytes = pack->ParameterBytes;
    byte* ResponseBytes = pack->ResponseBytes;
    byte* RawBytes = pack->RawBytes;
    //CheckParsing(buffer[0], COMMAND_START_CODE_1, COMMAND_START_CODE_1, "COMMAND_START_CODE_1");
    //CheckParsing(buffer[1], COMMAND_START_CODE_2, COMMAND_START_CODE_2, "COMMAND_START_CODE_2");
    //CheckParsing(buffer[2], COMMAND_DEVICE_ID_1, COMMAND_DEVICE_ID_1, "COMMAND_DEVICE_ID_1");
    //CheckParsing(buffer[3], COMMAND_DEVICE_ID_2, COMMAND_DEVICE_ID_2, "COMMAND_DEVICE_ID_2");
    CheckParsing(buffer[8], 0x30, 0x31, "AckNak_LOW");
    if (buffer[8] == 0x30) {
        ACK = 1;
    }
    else
        ACK = 0;
    
    CheckParsing(buffer[9], 0x00, 0x00, "AckNak_HIGH");
    
    word checksum = CalculateChecksumIn(buffer, 10);
    byte checksum_low = GetLowByte(checksum);
    byte checksum_high = GetHighByte(checksum);
    CheckParsing(buffer[10], checksum_low, checksum_low, "Checksum_LOW");
    CheckParsing(buffer[11], checksum_high, checksum_high, "Checksum_HIGH");
    
    //int Error = ParseFromBytes(buffer[5], buffer[4]);
    
    ParameterBytes[0] = buffer[4];
    ParameterBytes[1] = buffer[5];
    ParameterBytes[2] = buffer[6];
    ParameterBytes[3] = buffer[7];
    ResponseBytes[0]=buffer[8];
    ResponseBytes[1]=buffer[9];
    for (int i=0; i < 12; i++)
    {
        RawBytes[i]=buffer[i];
    }
}

word CalculateChecksumIn(byte* buffer, int length)
{
    word checksum = 0;
    for (int i=0; i<length; i++)
    {
        checksum +=buffer[i];
    }
    return checksum;
}

void input_cmd(byte cmd, byte* arr ) {
    arr[0] = 0x00;
    arr[1] = cmd;
}

void open() {
    
    struct command_packet cp;
    input_cmd(Open, cp.command);
    cp.Parameter[0] = 0x00;
    cp.Parameter[1] = 0x00;
    cp.Parameter[2] = 0x00;
    cp.Parameter[3] = 0x00;
    byte* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    
    return ;
}

void close()
{
    
    struct command_packet cp;
    input_cmd(Close, cp.command);
    cp.Parameter[0] = 0x00;
    cp.Parameter[1] = 0x00;
    cp.Parameter[2] = 0x00;
    cp.Parameter[3] = 0x00;
    byte* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    
}

void ParameterFromInt(int i, byte* Parameter)
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
       	while (!(SCISR1 & 0x80));
        SCIDRL = cmd[i];
        }
        
    }
    
    
}

struct response_packet* GetResponse() {
	static struct response_packet rp;
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
    unsigned char para[4];
    unsigned char resp[2];
    para[0] = packet[4];
    para[1] = packet[5];
    para[2] = packet[6];
    para[3] = packet[7];
    resp[0]=  packet[8];
    resp[1]=  packet[9];
    rp.ParameterBytes = para;
    rp.ResponseBytes = resp;
    
	return &rp;
}


int is_press_finger()
{
    
    struct command_packet cp;
    input_cmd(IsPressFinger, cp.command);
    //printf("%x%x", cp.command[0], cp.command[1]);
    cp.Parameter[0] = 0x00;
    cp.Parameter[1] = 0x00;
    cp.Parameter[2] = 0x00;
    cp.Parameter[3] = 0x00;
    byte* packetbytes = GetPacketBytes(&cp);
    //printf("packet bytes:\n");
    //for (int i = 0; i < 12; i++) {
    //	printf("Parameter[%d]: %x\n", i, packetbytes[i]);
    //}
    
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    int retval = 0;
    int pval = rp->ParameterBytes[0];
    pval += rp->ParameterBytes[1];
    pval += rp->ParameterBytes[2];
    pval += rp->ParameterBytes[3];
    if (pval == 0)
        retval = 1;
    return retval;
    return 0;
}

int ChangeBaudRate(unsigned long baud)
{
    if ((baud == 9600) || (baud == 19200) || (baud == 38400) || (baud == 57600) || (baud == 115200))
    {
        
        
        struct command_packet cp;
        input_cmd(Close, cp.command);
        ParameterFromInt(baud, cp.Parameter);
        byte* packetbytes = GetPacketBytes(&cp);
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
    struct command_packet cp;
    input_cmd(CmosLed, cp.command);
    if (on)
    {
        cp.Parameter[0] = 0x01;
    }
    else
    {
        
        cp.Parameter[0] = 0x00;
    }
    cp.Parameter[1] = 0x00;
    cp.Parameter[2] = 0x00;
    cp.Parameter[3] = 0x00;
    byte* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    int retval = 1;
    if (ACK == 0)
        retval = 0;
    return retval;
}

int IntFromParameter(byte* ParameterBytes) {
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
    byte* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    
    int retval = IntFromParameter(rp->ParameterBytes);
    
    return retval;
}

int check_enrolled(int id)
{
    
    struct command_packet cp;
    input_cmd(CheckEnrolled, cp.command);
    ParameterFromInt(id, cp.Parameter);
    byte* packetbytes = GetPacketBytes(&cp);
    
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
    byte* packetbytes = GetPacketBytes(&cp);
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
    byte* packetbytes = GetPacketBytes(&cp);
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
    byte* packetbytes = GetPacketBytes(&cp);
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
    byte* packetbytes = GetPacketBytes(&cp);
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
    byte* packetbytes = GetPacketBytes(&cp);
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
    byte* packetbytes = GetPacketBytes(&cp);
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
    byte* packetbytes = GetPacketBytes(&cp);
    SendCommand(packetbytes, 12);
    struct response_packet* rp = GetResponse();
    int retval = ACK;
    return retval;
}








