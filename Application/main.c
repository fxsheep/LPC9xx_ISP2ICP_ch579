//***************************************************************************
//* 								ISP_to_ICP_bridge.c 
//*	By : Bauke Siderius		
//*	Discription : ISP/ICP gateway code for LPC900 programmer using MCB900 
//*
//***************************************************************************
//* Versions
//***************************************************************************
//*
//* v1.6	October 2005
//*			Fixed the program command, a load command has to be given before
//*	  		putting Data in FM_DATA
//*
//* v1.5	September 2005
//*			Added support for the Clear Configuration Protection	
//*
//* v1.4 	December 2004
//*			Erase sector and erase page fix.
//*
//* v1.3	March 2004
//*			Increased data buffer size to 64 to support LPC92x in FlashMagic.	
//*
//* v1.2	August 2003
//*			Added a delay before entering ICP.	
//*
//* v1.1 	August 2003
//*			Put ports in input only when not in use.
//*			Fixed CRC read out.
//*
//* v1.0 	May 2003
//* 		Initial version.
//*
//***************************************************************************
#include "CH57x_common.h"
#include "progdef.h"
#include <stdio.h>

//***************************************************************************
//* variables used for passing parameters
//***************************************************************************
unsigned char reg7;
//***************************************************************************
//* buffers for read record
//***************************************************************************
unsigned char address_low = 0;					// low address
unsigned char address_high = 0;					// high address
unsigned char checksum = 0;						// checksum
unsigned char nbytes = 0;						// number of data bytes
unsigned char record_type = 0;					// record type
unsigned char data_bytes[64];				// data buffer
unsigned char program_byte;						// byte to be programmed

//***************************************************************************
//* init()
//* Input(s) : none.
//* Returns : none.
//* Description : initialization of P89LPC932
//***************************************************************************
void init(void)
{
	/* GPIO for ICP */
	GPIOA_ResetBits(ICP_VCC);
	GPIOA_ModeCfg(ICP_VCC, GPIO_ModeOut_PP_20mA);
	GPIOA_ModeCfg(ICP_RESET | ICP_PCL | ICP_PDA, GPIO_ModeOut_PP_5mA);

	/* GPIO for UART */
  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
	GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);

	/* ISP UART */
  UART1_DefInit();
	UART1_BaudRateCfg(19200);
}

//***************************************************************************
//* main()
//* Input(s) : none.
//* Returns : none.
//* Description : main loop
//***************************************************************************
int main(void)
{
	unsigned char index;
  init();
  msec(500);									// delay 500 msec to sabalize before entering ICP mode
  enter_icp();									// go into ICP mode
  while(1)										// HexFile Loader
  {		
    checksum = 0;								// clear checksum before loading file
    while(echo() != ':');						// record starts with a ':'	
    nbytes = get2();							// get number of bytes in record
    address_high = get2();						// get MSB of load address
    address_low = get2();						// get LSB of load address
    record_type = get2();						// get record type
    for(index=0; index < nbytes; index++)		// read record data to buffer
    {
      data_bytes[index] = get2();				// put databytes in an array
    }
    reg7 = checksum;							// put calculated checksum in reg7
    if (reg7 != get2())							// read and check checksum on record
    {
      printf("X\r\n");							// print error message					
      continue;									// restart HexLoader
    }
    switch(record_type)							// switch on record type
    {
      case PROGRAM:								// program record type
      {
        program_record();						//
        printf(".\r\n");							// send ok message
	      break;
	    }
	    case READ_VERSION:						// read version record type
	    {
	      print_hex_to_ascii(ISP_VERSION);		// send ISP version
	      print_hex_to_ascii(ICP_VERSION);		// send ICP version
        printf(".\r\n");							// send ok message     
	      break;
	    }
	    case MISC_WRITE:							// misc write record type
	    { 
        write_config();							// write config byte
        printf(".\r\n");							// send ok message
	      break;
	    }
	    case MISC_READ:							// misc read record type
	    {
        read_config();							// read config byte
	    	print_hex_to_ascii(data_bytes[0]);		// send config byte in ascii
        printf(".\r\n");							// send ok message
	      break;
	    }
	    case ERASE:								// erase record type
	    {
        if(data_bytes[0] == 0)					// check if dbytes[0] indicates page erase
        {
	        erase_page();							// try erase a page and get status
          printf(".\r\n");						// send ok message
		    }  
		    else if(data_bytes[0] == 1)				// check if dbytes[0] indicates sector erase
		    {
  		    erase_sector();						// try erase a sector and get status
          printf(".\r\n");						// send ok message
		    }
		    else
        {
          printf("R\r\n");						// send error message
        }
	      break;
	    }
      case SECTOR_CRC:							// sector CRC record type
      {
        crc_sector();							// try sector CRC and get status
        print_hex_to_ascii(data_bytes[3]);		// send first CRC in ascii
        print_hex_to_ascii(data_bytes[2]);		// send second CRC in ascii
        print_hex_to_ascii(data_bytes[1]);		// send third CRC in ascii
        print_hex_to_ascii(data_bytes[0]);		// send fourth CRC in ascii
        printf(".\r\n");							// send ok message
        break;
      }
      case GLOBAL_CRC:
      {  	
        crc_global();
        print_hex_to_ascii(data_bytes[3]);		// send first CRC in ascii
        print_hex_to_ascii(data_bytes[2]);		// send second CRC in ascii
        print_hex_to_ascii(data_bytes[1]);		// send third CRC in ascii
        print_hex_to_ascii(data_bytes[0]);		// send fourth CRC in ascii
        printf(".\r\n");							// send ok message
        break;
	    }
      case CHIP_ERASE:							// chip erase record type	
      {
        erase_global();							// try erase global and get status
        printf(".\r\n");							// send ok message
	      break;
	    }
      default:									// incorrect record type	
      {
        printf("R\r\n");							// send error message
        break;
      }	  
	  }
  }	
}

//***************************************************************************
//* enter_icp()
//* Input(s) : none.
//* Returns : none.
//* Description : function to pulse reset to enter ICP mode
//***************************************************************************
void enter_icp(void)
{	
  char pulses = 0;

	/* Power off target */
	GPIOA_ResetBits(ICP_VCC | ICP_RESET);
	msec(10);
	/* Power on target with reset held low */
	GPIOA_SetBits(ICP_VCC);
	msec(5);
  for(pulses = 0; pulses < 7; pulses++)			// pulse reset 7 times
  {
    GPIOA_SetBits(ICP_RESET);
    DelayUs(30);			// wait about 15 usec
		GPIOA_ResetBits(ICP_RESET);
    DelayUs(30);			// wait about 15 usec
  }
  GPIOA_SetBits(ICP_RESET);									// hold reset high
}

//***************************************************************************
//* shift_out()
//* Input(s) : data_byte.
//* Returns : none.
//* Description : function to shift out data to the part being programmed
//***************************************************************************
void shift_out(char data_byte)
{
  char shift_bit;	
  char temp_byte = 0;
	GPIOA_ModeCfg(ICP_PDA, GPIO_ModeOut_PP_5mA);
  temp_byte = data_byte;						// put databyte in a temp byte
  for(shift_bit = 0; shift_bit < 8; shift_bit++)// shift out 8 bits
  {
    GPIOA_ResetBits(ICP_PCL);									// hold clock line low
    if(temp_byte & 0x01)						// check if LSB is set
    {
      GPIOA_SetBits(ICP_PDA);									// set data-line high
    }
    else
    {
      GPIOA_ResetBits(ICP_PDA);									// set data-line low
    }
	temp_byte = (data_byte >>= 1); 				// shift databyte right one, put in temp
	GPIOA_SetBits(ICP_PCL);									// clock  databit
  }
  GPIOA_SetBits(ICP_PDA);										// set dataline high after transfer
  GPIOA_SetBits(ICP_PCL);										// hold dataline high after transfer
}	

//***************************************************************************
//* shift_in()
//* Input(s) : none.
//* Returns : Data shifted back from part that is being programmed.
//* Description : function read data from the part being programmed
//***************************************************************************
char shift_in(void)
{
  char data_byte = 0;
  char shift_bit = 0;
  GPIOA_ResetBits(ICP_PCL);
  GPIOA_ModeCfg(ICP_PDA, GPIO_ModeIN_Floating);
  for(shift_bit = 0; shift_bit < 8; shift_bit++)// shift in 8 bits
  {
		GPIOA_SetBits(ICP_PCL);									// clock out databit
	if(GPIOA_ReadPortPin(ICP_PDA))										// check if PDA is set	
	{
	  data_byte |= 0x80;						// set bit
    }
    else
	{
  	  data_byte &= ~0x80;						// or clear bit
    }
    if(shift_bit < 7)							// don't shift last bit MSB
    {	
 	  data_byte >>= 1;
    }
	GPIOA_ResetBits(ICP_PCL);									// hold clock line low
  }
  GPIOA_ResetBits(ICP_PCL);										// set clockline low after transfer
  GPIOA_SetBits(ICP_PDA);										// hold dataline high
	GPIOA_ModeCfg(ICP_PDA, GPIO_ModeOut_PP_5mA);
  return data_byte;								// return clocked in bit
}

//***************************************************************************
//* program()
//* Input(s) : none.
//* Returns : none.
//* Description : funtion to program the page register into Flash
//***************************************************************************
void program(void)
{
  unsigned char read_data = 0;
  shift_out(WR_FMADRL);							// write address low command page aligned
  shift_out(address_low);						// write address from the isp command
  shift_out(WR_FMADRH);							// write address high command
  shift_out(address_high);						// write address from the isp command
  shift_out(WR_FMCON);							// write to FMCON
  shift_out(LOAD);								// load commmand 
  shift_out(WR_FMDATA);							// write to FMDATA
  shift_out(program_byte);						// load databyte
  shift_out(WR_FMCON);							// write to FMCON
  shift_out(PROG);								// program commmand
  do
  {
    shift_out(RD_FMCON);						// read FMCON command
	read_data = shift_in();						// shift in data from FMCON
  }
  while(read_data & 0x80);						// check for done status MSB
}

//***************************************************************************
//* erase_global()
//* Input(s) : none.
//* Returns : none.
//* Description : function to erase all Flash and config
//***************************************************************************
void erase_global(void)
{
  unsigned char read_data = 0;
  char index, dummy;
  shift_out(WR_FMCON);							// write FMCON command
  shift_out(ERS_G);								// write erase global command
//***************************************************************************
//* This code must be added due to a bug in erase global
//* otherwise it will stay busy forever
//***************************************************************************
  for(index = 0; index < 50; index++)			// read for about 5ms
  {
    shift_out(RD_FMDATA_I);						// read data increment address
    dummy = shift_in();							// do dummy reads
  } 
//***************************************************************************
//* End added code for erase global bug
//***************************************************************************
  do										
  {
    shift_out(RD_FMCON);						// read FMCON command
	read_data = shift_in();						// shift in data from FMCON
  }
  while(read_data & 0x80);						// check for done status MSB 
}

//***************************************************************************
//* erase_sector()
//* Input(s) : none.
//* Returns : none.
//* Description : function to erase a sector in Flash
//***************************************************************************
void erase_sector(void)
{
  unsigned char read_data = 0;
  shift_out(WR_FMADRH);							// write address high command
  shift_out(data_bytes[1]);						// write address stripped from the isp command
  shift_out(WR_FMCON);							// write to FMCON
  shift_out(ERS_S);								// write erase sector command
  do
  {
    shift_out(RD_FMCON);						// read FMCON command
	read_data = shift_in();						// shift in data from FMCON
  }
  while(read_data & 0x80);						// check for done status MSB   
}

//***************************************************************************
//* erase_page()
//* Input(s) : none.
//* Returns : none.
//* Description : function to erase a page in Flash
//***************************************************************************
void erase_page(void)
{
  unsigned char read_data = 0;					// declare local read_data variable
  shift_out(WR_FMADRL);							// write address low command
  shift_out(data_bytes[2]);						// write address stripped from the isp command
  shift_out(WR_FMADRH);							// write address high command
  shift_out(data_bytes[1]);						// write address stripped form the isp command
  shift_out(WR_FMCON);							// write to FMCON
  shift_out(ERS_P);								// write erase page command
  do											// loop till status is done
  {
    shift_out(RD_FMCON);						// read FMCON command
	read_data = shift_in();						// shift in data from FMCON
  }
  while(read_data & 0x80);						// check for done status MSB     
}

//***************************************************************************
//* crc_global()
//* Input(s) : none.
//* Returns : none.
//* Description : function to read the global CRC
//***************************************************************************
void crc_global(void)
{
  unsigned char read_data = 0;					// declare local read_data variable
  unsigned char crc_index = 0;					// declare local crc_index variable
  shift_out(WR_FMCON);							// write to FMCON
  shift_out(CRC_G);								// write global CRC command
  do
  {
    shift_out(RD_FMCON);						// read FMCON command
	read_data = shift_in();						// shift in data from FMCON
  }	
  while(read_data & 0x80);						// check for done status MSB
  for(crc_index = 0; crc_index < 4; crc_index++)// read 4 CRC bytes
  {
    shift_out(RD_FMDATA_I);						// write read data and increment command    
    data_bytes[crc_index] = shift_in();			// read CRC bytes
  }    
}

//***************************************************************************
//* crc_sector()
//* Input(s) : none.
//* Returns : none.
//* Description : function to read the CRC of a sector
//***************************************************************************
void crc_sector(void)
{
  unsigned char read_data = 0;					// declare local read_data variable
  unsigned char crc_index = 0;					// declare local crc_index variable
  shift_out(WR_FMADRH);							// write address high command
  shift_out(data_bytes[0]);						// write address stripped form the isp command
  shift_out(WR_FMCON);							// write to FMCON
  shift_out(CRC_S);								// write sector CRC command
  do
  {
    shift_out(RD_FMCON);						// read FMCON command
	read_data = shift_in();						// shift in data from FMCON
  }
  while(read_data & 0x80);						// check for done status MSB
  for(crc_index = 0; crc_index < 4; crc_index++)// read 4 CRC bytes
  {
    shift_out(RD_FMDATA_I);						// write read data and increment command    
    data_bytes[crc_index] = shift_in();			// read CRC bytes
  }  
}

//***************************************************************************
//* read_config()
//* Input(s) : none.
//* Returns : none.
//* Description : function to read from the configuration space
//***************************************************************************
void read_config(void)
{
  shift_out(WR_FMCON);							// write to FMCON
  shift_out(CONF);								// write acces config command
  shift_out(WR_FMADRL);							// write address low command
  shift_out(data_bytes[0]);						// write address stripped from the isp command
  shift_out(RD_FMDATA);							// read FMDATA command
  data_bytes[0] = shift_in();					// read config
}

//***************************************************************************
//* write_config()
//* Input(s) : none.
//* Returns : none.
//* Description : function to write to the configuration space
//***************************************************************************
void write_config(void)
{
  unsigned char read_data = 0;
  if(data_bytes[0] == 0x10)
  {
    shift_out(WR_FMCON);						// write to FMCON
    shift_out(CCP);								// write clear config protection
    shift_out(WR_FMDATA);						// write FMDATA command
    shift_out(CLR_CCP_KEY);						// wirte clear config protection key command
    do											// loop while status is busy
    {
      shift_out(RD_FMCON);						// read FMCON command
	  read_data = shift_in();					// shift in data from FMCON
    }
    while(read_data & 0x80);					// check for done status MSB     
  }
  else
  {
    shift_out(WR_FMCON);						// write to FMCON
    shift_out(CONF);							// write acces config command
    shift_out(WR_FMADRL);						// write address low command
    shift_out(data_bytes[0]);					// write address stripped from the isp command
    shift_out(WR_FMDATA);						// write FMDATA command
    shift_out(data_bytes[1]);					// wirte value stripped from the isp command
    do											// loop while status is busy
    {
      shift_out(RD_FMCON);						// read FMCON command
	  read_data = shift_in();					// shift in data from FMCON
    }
    while(read_data & 0x80);					// check for done status MSB 
  }    
}

//***************************************************************************
//* echo()
//* Input(s) : none.
//* Returns : character read from ISP record.
//* Description : check if in ICP mode
//***************************************************************************
unsigned char echo()
{
  unsigned char ch;
  while(!R8_UART1_RFC); // wait until reception is complete	
  ch = UART1_RecvByte();															// read UART buffer		
  while(R8_UART1_TFC);	// wait until previous transmission is complete
	UART1_SendByte(ch);																	// send character
  if (ch&0x40)	ch&=0xDF;						// if character, then make it upper case
  return ch;
}

//***************************************************************************
//* get2()
//* Input(s) : none.
//* Returns : byte read from the ISP record.
//* Description : check if in ICP mode
//***************************************************************************
unsigned char get2()
{
  unsigned char record_byte;		
  record_byte = ascii_to_hex(echo())<<4;		// read high nibble
  record_byte += ascii_to_hex(echo());			// read low nibble
  checksum -= record_byte;						// update checksum
  return record_byte;							// return byte
}

//***************************************************************************
//* ascii_to_hex()
//* Input(s) : character in ascii format
//* Returns : character in hex format
//* Description : converts ascii to hex
//***************************************************************************
unsigned char ascii_to_hex(unsigned char ch)
{
  if (ch & 0x40)								// convert ASCII character
  {	
    ch += 0x09;
  }
  ch &= 0x0F;									// into 2 digit Hex
  return ch;
}

//***************************************************************************
//* print_hex_to_ascii()
//* Input(s) : character
//* Returns : none.
//* Description : converts hex to ascii and print the character to the UART
//***************************************************************************
void print_hex_to_ascii(unsigned char ch)
{
  char temp_character;
  temp_character = ch;
  temp_character >>= 4;							// get highest nible
  temp_character &= 0x0F;						// save lower nibble
  if (temp_character >= 10)						// check if A-F
  {
    temp_character += 7;						// offset for A-F
  }
  temp_character += 0x30;						// add '0' to get ascii
  printf("%c", temp_character);					// print upper nible
  temp_character = ch;
  temp_character &= 0x0F;						// save lower nibble
  if (temp_character >= 10)						// check if A-F
  {
    temp_character += 7;						// offset for A-F
  }
  temp_character += 0x30;						// add '0' to get ascii
  printf("%c", temp_character);					// print lower nible
}

//***************************************************************************
//* program_record()
//* Input(s) : none.
//* Returns : none.
//* Description : function to program bytes from hex record
//* 				Warning the hex file records need to be page aligned 
//***************************************************************************
void program_record(void)		 				
{
  char index;
  for(index = 0; index < nbytes; index++)		// program all individual bytes
  {
    program_byte = data_bytes[index];			// get byte to be programmed
    program();									// program the byte
    address_low++;								// update address for programmed byte
    if(address_low == 0x00)						// check if low address rolls over
    {
      address_high += 1;						// then increment high address
    }   
  }    
}

