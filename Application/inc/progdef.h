//***************************************************************************
//* 								progdef.h 
//*	By : Bauke Siderius		
//*	Discription : defenitions for programming commands for the LPC90x 
//***************************************************************************

//***************************************************************************
//* Pin Definitions
//***************************************************************************
//#define ICP_PORT		GPIOA				// define GPIO port for ICP
#define ICP_VCC			GPIO_Pin_2	// define VCC control pin
#define ICP_RESET		GPIO_Pin_3	// define RESET control pin
#define ICP_PCL			GPIO_Pin_4	// define PCL pin
#define ICP_PDA			GPIO_Pin_5	// define PDA pin

//***************************************************************************
//* Programmer Opcodes
//***************************************************************************
#define NOP				0x00		// No operation
#define WR_FMADRL		0x08		// Write address low
#define RD_FMADRL		0x09		// Read address low 
#define WR_FMADRH		0x0A		// Write address high
#define RD_FMADRH		0x0B		// Read address low
#define WR_FMCON		0x0E		// Write a command to FMCON
#define RD_FMCON		0x0F		// Read a command from FMCON
#define WR_FMDATA_PG	0x14		// Write a command to FMDATA and inc addr
#define RD_FMDATA_PG	0x15		// Read a coomand from FMDATA and inc addr
#define WR_FMDATA		0x0C		// Write a data byte to FMDATA
#define RD_FMDATA		0x0D		// Read a data byte from FMDATA
#define WR_FMDATA_I		0x04		// Write a data byte to FMDATA and increment
#define RD_FMDATA_I		0x05		// Read a data byte from FMDATA and increment
#define CLR_CCP_KEY		0x96		// Clear configuration protection key

//***************************************************************************
//* Programmer Commands
//***************************************************************************
#define LOAD			0x00		// Clear then load the page register
#define PROG			0x48		// Program page with page register command
#define ERS_G			0x72		// Erase global command 
#define ERS_S			0x71		// Erase sector command
#define ERS_P			0x70		// Erase page comand
#define CONF			0x6C		// Access user config
#define CRC_G			0x1A		// Global CRC
#define CRC_S			0x19		// Sector CRC
#define CRC_A			0x14		// CRC user config
#define CCP				0x67		// Clear Configuration Protection

//***************************************************************************
//* ISP function codes
//***************************************************************************
#define PROGRAM			0			// program user code
#define READ_VERSION	1			// read IAP/ISP version
#define MISC_WRITE		2			// misc. write functions
#define MISC_READ		3			// misc. read functions
#define ERASE			4			// erase sector/page
#define SECTOR_CRC		5			// read sector CRC
#define GLOBAL_CRC		6			// read global CRC
#define LOAD_BAUD		7			// not implemented
#define CHIP_ERASE		9			// full chip erase 	`

//***************************************************************************
//* versions of ISP and ICP
//***************************************************************************
#define ISP_VERSION		0x06		// ICP version
#define ICP_VERSION		0x06		// ICP version

//***************************************************************************
//* Functions
//***************************************************************************
void init(void);	              				// part initialization
//***************************************************************************
//* ICP Functions
//***************************************************************************
void enter_icp(void);
void init_flash(void);
void load_page(void);
void program(void);
void erase_global(void);
void erase_sector(void);
void erase_page(void);
void crc_global(void);
void crc_sector(void);
void write_config(void);
void read_config(void);
void shift_out(char data_byte);
char shift_in(void);
//***************************************************************************
//* ISP Functions
//***************************************************************************
void program_record(void);
unsigned char echo();
unsigned char get2();
unsigned char ascii_to_hex(unsigned char ch);
void print_hex_to_ascii(unsigned char ch);

#define msec DelayMs

