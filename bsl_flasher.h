/*
* @Author Michael Brooks
* GPL
*
*/

typedef unsigned char   u8;
typedef signed int      s32;

/* Chip Addresses */
#define MSP_ADDR 0x48
#define BSL_ADDR 0x70

/* BSL Commands from Host */
#define RX_BLOCK 0x10
#define RX_PWD 0x11
#define ERASE_SEG 0x12
#define UNLOCK 0x13
#define MASS_ERASE 0x15
#define CRC_CHECK 0x16
#define LOAD_PC 0x17
#define TX_BLOCK 0x18
#define TX_BSL_VER 0x19
#define TX_BUFFER_SIZE 0x1A
#define RESET 0x1C

/* BSL Response Commands */
#define RESPONSE 0x3A //This can be a data block, BSL version, CRC value, or buffer size based on called function
#define MESSAGE 0x3B

/* BSL Responses Messages */
#define SUCCESS 0x0
#define CRC_FAIL 0x1
#define FLASH_FAIL 0x2
#define VOLTAGE_CHG 0x3
#define BSL_LOCKED 0x4
#define PWD_FAIL 0x5
#define BYTE_WRITE_FAIL 0x6
#define UNKNOWN_CMD 0x7
#define SIZE_EXCEEDED 0x8

/* Other BSL Macros */
#define HEADER 0x80
char* CLEAN_PWD="FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";

/* Function Declerations */
int send_command(int file, char NH, char NL, char CMD, char AL, char AM, char AH, char* DATA);

int get_response(int file, char *ret_buf);

int jump_to_bsl();

int flash_image(int file, char* argv);

unsigned short get_CRC(int size, char* buf);
