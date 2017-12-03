//
//  serial_com.h
//  Fingerprint Library
//
//  Created by Sam Jenkins on 12/1/17.
//
//

#ifndef serial_com_h
#define serial_com_h

// command code definitions
#define NotSet               0x00		// Default value for enum. Scanner will return error if sent this.
#define Open				 0x01		// Open Initialization
#define Close				 0x02		// Close Termination
#define ChangeEBaudRate		 0x04		// ChangeBaudrate Change UART baud rate
#define CmosLed				 0x12		// CmosLed Control CMOS LED
#define GetEnrollCount		 0x20		// Get enrolled fingerprint count
#define CheckEnrolled		 0x21		// Check whether the specified ID is already enrolled
#define EnrollStart			 0x22		// Start an enrollment
#define Enroll1				 0x23		// Make 1st template for an enrollment
#define Enroll2				 0x24		// Make 2nd template for an enrollment
#define Enroll3				 0x25		// Make 3rd template for an enrollment, merge three templates into one template, save merged template to the database
#define IsPressFinger		 0x26		// Check if a finger is placed on the sensor
#define DeleteID			 0x40		// Delete the fingerprint with the specified ID
#define DeleteAll			 0x41	// Delete all fingerprints from the database
#define Verify1_1			 0x50		// Verification of the capture fingerprint image with the specified ID
#define Identify1_N			 0x51		// Identification of the capture fingerprint image with the database
#define VerifyTemplate1_1	 0x52		// Verification of a fingerprint template with the specified ID
#define IdentifyTemplate1_N	 0x53	// Identification of a fingerprint template with the database
#define CaptureFinger		 0x60		// Capture a fingerprint image(256x256) from the sensor
#define MakeTemplate		 0x61		// Make template for transmission
#define GetImage             0x62		// Download the captured fingerprint image(256x256)
#define GetRawImage			 0x63		// Capture & Download raw fingerprint image(320x240)
#define GetTemplate			 0x70		// Download the template of the specified ID
#define SetTemplate			 0x71		// Upload the template of the specified ID
#define GetDatabaseStart	 0x72		// Start database download, obsolete
#define GetDatabaseEnd		 0x73		// End database download, obsolete
#define Ack					 0x30		// Acknowledge.
#define Nack				 0x31		// Non-acknowledge

//
#define COMMAND_START_CODE_1 0x55	// Static byte to mark the beginning of a command packet	-	never changes
#define COMMAND_START_CODE_2 0xAA	// Static byte to mark the beginning of a command packet	-	never changes
#define COMMAND_DEVICE_ID_1  0x01	// Device ID Byte 1 (lesser byte)							-	theoretically never changes
#define COMMAND_DEVICE_ID_2  0x00

// error code definitions
#define NO_ERROR					 0x0000	// Default value. no error
#define NACK_TIMEOUT				 0x1001	// Obsolete, capture timeout
#define NACK_INVALID_BAUDRATE		 0x1002	// Obsolete, Invalid serial baud rate
#define NACK_INVALID_POS			 0x1003	// The specified ID is not between 0~199
#define NACK_IS_NOT_USED			 0x1004	// The specified ID is not used
#define NACK_IS_ALREADY_USED	     0x1005	// The specified ID is already used
#define NACK_COMM_ERR				 0x1006	// Communication Error
#define NACK_VERIFY_FAILED			 0x1007	// 1:1 Verification Failure
#define NACK_IDENTIFY_FAILED		 0x1008	// 1:N Identification Failure
#define NACK_DB_IS_FULL				 0x1009	// The database is full
#define NACK_DB_IS_EMPTY			 0x100A	// The database is empty
#define NACK_TURN_ERR			     0x100B	// Obsolete, Invalid order of the enrollment (The order was not as: EnrollStart -> Enroll1 -> Enroll2 -> Enroll3)
#define NACK_BAD_FINGER				 0x100C	// Too bad fingerprint
#define NACK_ENROLL_FAILED			 0x100D	// Enrollment Failure
#define NACK_IS_NOT_SUPPORTED		 0x100E	// The specified command is not supported
#define NACK_DEV_ERR			     0x100F	// Device Error, especially if Crypto-Chip is trouble
#define NACK_CAPTURE_CANCELED		 0x1010	// Obsolete, The capturing is canceled
#define NACK_INVALID_PARAM			 0x1011	// Invalid parameter
#define NACK_FINGER_IS_NOT_PRESSED	 0x1012	// Finger is not pressed
#define INVALID						 0XFFFF	// Used when parsing fails


struct command_packet {
    unsigned char Parameter[4]; // 4 bytes
    unsigned char command[2];  // 2 bytes

};

struct response_packet {
    unsigned char* RawBytes;
    unsigned char* ParameterBytes;
    unsigned char* ResponseBytes;

};

unsigned char* GetPacketBytes();							// returns the bytes to be transmitted
void ParameterFromInt(int i, unsigned char* Parameter);

unsigned int _CalculateChecksumOut(struct command_packet* pack);						// Checksum is calculated using byte addition
unsigned char GetHighByte(unsigned int w);
unsigned char GetLowByte(unsigned int w);


//Initialises the device and gets ready for commands
void open();

// Does not actually do anything (according to the datasheet)
// I implemented open, so had to do closed too... lol
void close();

// Turns on or off the LED backlight
// LED must be on to see fingerprints
// Parameter: true turns on the backlight, false turns it off
// Returns: True if successful, false if not
int SetLED(int on);

// Changes the baud rate of the connection
// Parameter: 9600 - 115200
// Returns: True if success, false if invalid baud
// NOTE: Untested (don't have a logic level changer and a voltage divider is too slow)
int ChangeBaudRate(unsigned long baud);

// Gets the number of enrolled fingerprints
// Return: The total number of enrolled fingerprints
int get_enroll_count();

// checks to see if the ID number is in use or not
// Parameter: 0-199
// Return: True if the ID number is enrolled, false if not
int check_enrolled(int id);

// Starts the Enrollment Process
// Parameter: 0-199
// Return:
//	0 - ACK
//	1 - Database is full
//	2 - Invalid Position
//	3 - Position(ID) is already used
int enroll_start(int id);

// Gets the first scan of an enrollment
// Return:
//	0 - ACK
//	1 - Enroll Failed
//	2 - Bad finger
//	3 - ID in use
int enroll1();

// Gets the Second scan of an enrollment
// Return:
//	0 - ACK
//	1 - Enroll Failed
//	2 - Bad finger
//	3 - ID in use
int enroll2();

// Gets the Third scan of an enrollment
// Finishes Enrollment
// Return:
//	0 - ACK
//	1 - Enroll Failed
//	2 - Bad finger
//	3 - ID in use
int enroll3();

// Checks to see if a finger is pressed on the FPS
// Return: true if finger pressed, false if not
int is_press_finger();

// Deletes the specified ID (enrollment) from the database
// Returns: true if successful, false if position invalid
int delete_ID(int ID);

// Deletes all IDs (enrollments) from the database
// Returns: true if successful, false if db is empty
int delete_all();

// Checks the currently pressed finger against a specific ID
// Parameter: 0-199 (id number to be checked)
// Returns:
//	0 - Verified OK (the correct finger)
//	1 - Invalid Position
//	2 - ID is not in use
//	3 - Verified FALSE (not the correct finger)
int verify1_1(int id);

// Checks the currently pressed finger against all enrolled fingerprints
// Returns:
//	0-199: Verified against the specified ID (found, and here is the ID number)
//	200: Failed to find the fingerprint in the database
int identify1_N();

// Captures the currently pressed finger into onboard ram
// Parameter: true for high quality image(slower), false for low quality image (faster)
// Generally, use high quality for enrollment, and low quality for verification/identification
// Returns: True if ok, false if no finger pressed
int capture_finger(int highquality);


int CheckParsing(unsigned char b, unsigned char propervalue, unsigned char alternatevalue);
void Response_Packet(unsigned char* buffer, struct response_packet* pack);
unsigned int CalculateChecksumIn(unsigned char* buffer, int length);
unsigned int byte_to_word(unsigned char* arr);
void input_cmd(unsigned char cmd, unsigned char* arr );
void SendCommand(unsigned char cmd[], int length);
struct response_packet* GetResponse();
int IntFromParameter(unsigned char*);

#endif /* serial_com_h */
