/*
* Insert GPL here
* @author Michael Brooks
* This loader is designed for basic I2C transfer, following the app note for the F522x BSL
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "bsl_flasher.h"



int main(int argc,char *argv[])
{
	int file, ret;
	unsigned int i2cbus;
	char filename[20];
	
	/*Check arguments*/
	if(argc!=3){
		printf("Wrong number of arguments\n");
		goto args_err;}
	
	ret = sscanf(argv[1], "%u", &i2cbus);
	if(ret!=1){
		printf("Something with argv[1] %u %u", argv[1], argv[2]);
		goto args_err;}

	
	/*Let's start by opening the I2C bus that was passed as an argument */
        sprintf(filename,"/dev/i2c-%d",i2cbus);
        if ((file = open(filename,O_RDWR)) < 0) {
                printf("Failed to open the bus.");
                exit(1);
        }

	/* Let's jump to BSL mode */
	jump_to_bsl(file);

	/* Now we open the BSL address */
	if (ioctl(file,I2C_SLAVE_FORCE,BSL_ADDR) < 0) {
                printf("Failed to acquire bus access and/or talk to slave.\n");
                /* ERROR HANDLING; you can check errno to see what went wrong */
                printf(strerror(errno));
                exit(1);
        }
		
	/* We send/recieve information according to the BSL appnote */

	ret = send_command(file, 0x1, 0x0, UNLOCK, NULL, NULL, NULL, NULL);
	/* First we send a dummy RX Password for an empty device */
	ret = send_command(file, 0x1, 0x0, MASS_ERASE, NULL, NULL, NULL, NULL);
	ret = send_command(file, 0x21, 0x0, RX_PWD, NULL, NULL, NULL, CLEAN_PWD);
	//sleep(1);	
	/* Unlock the flash region */

	//ret = send_command(file, 0x1, 0x0, MASS_ERASE, NULL, NULL, NULL, NULL, 0x64, 0xA3);

	flash_image(file, argv[2]);
	sleep(10);
	ret = send_command(file, 0x1, 0x0, RESET, NULL, NULL, NULL, NULL);	

	printf("Complete!\n");
	return 0;

args_err:
	printf("BSL Flasher\nUsage: bsl_flasher <I2C Bus#> <Firmware image>\n");
	return 1;
	
}

int send_command(int file, char NL, char NH, char CMD, char AL, char AM, char AH, char* DATA){
	
	int ret, i, size;
	unsigned char buf[137];
	//printf("Reached send...\n");	
	
	/* The size of the buffer we send varies by the command used, as per table 6 in BSL app note, due to laziness we defined the
	   the largest possible block and then control the write size */
	unsigned short checksum = 0xFFFF;

	switch(CMD){
		case RX_BLOCK:
			//printf("Sending data block... ");
                        size = (NL + (NH << 8)) + 5;
			buf[0] = HEADER;
                        buf[1] = NL;
                        buf[2] = NH;
                        buf[3] = CMD;
			buf[4] = AL;
			buf[5] = AM;
			buf[6] = AH;
                        for(i=0; i < size-9; i++)
                                buf[7+i]=DATA[i];

                        checksum = get_CRC(size, buf);
                        buf[size-2] = (checksum & 0x00FF);
                        buf[size-1] = (checksum >> 8) & 0xff;
			break;
		case RX_PWD:
			size = sizeof(char)*38;
			printf("Setting RX Password... ");
			buf[0] = HEADER;
			buf[1] = NL;
			buf[2] = NH;
			buf[3] = CMD;
			for(i=0; i <32; i++)
				buf[4+i]=0xFF;
                        checksum = get_CRC(size, buf);
                        buf[36] = (checksum & 0x00FF);
                        buf[37] = (checksum >> 8) & 0xff;
                        break;
		case ERASE_SEG:
                        size = sizeof(char) * 9;
                        printf("Erasing Segment... ");
                        buf[0] = HEADER;
                        buf[1] = NL;
                        buf[2] = NH;
                        buf[3] = CMD;
			buf[4] = AL;
                        buf[5] = AM;
                        buf[6] = AH;
                        checksum = get_CRC(size, buf);
                        buf[7] = (checksum & 0x00FF);
                        buf[8] = (checksum >> 8) & 0xff;
			break;
                case UNLOCK:
			size = sizeof(char) * 6;
		        printf("Unlocking... ");
                        buf[0] = HEADER;
                        buf[1] = NL;
                        buf[2] = NH;
                        buf[3] = CMD;
			checksum = get_CRC(size, buf);
			buf[4] = (checksum & 0x00FF);
        	        buf[5] = (checksum >> 8) & 0xff;
                        break;
                case MASS_ERASE:
                        size = sizeof(char) * 6;
                        printf("Mass erase of flash... ");
                        buf[0] = HEADER;
                        buf[1] = NL;
                        buf[2] = NH;
                        buf[3] = CMD;
			checksum = get_CRC(size, buf);
			buf[4] = (checksum & 0x00FF);
                        buf[5] = (checksum >> 8) & 0xff;
                        break;
                case CRC_CHECK:
                        break;
                case LOAD_PC:
                        break;
	        case TX_BLOCK:
	                break;
                case TX_BSL_VER:
                        break;
                case TX_BUFFER_SIZE:
                        break;
                case RESET:
                        size = sizeof(char) * 6;
                        printf("Reseting device... ");
                        buf[0] = HEADER;
                        buf[1] = NL;
                        buf[2] = NH;
                        buf[3] = CMD;
                        checksum = get_CRC(size, buf);
                        buf[4] = (checksum & 0x00FF);
                        buf[5] = (checksum >> 8) & 0xff;
			break;
		default:
			break;
	}
	ret= write(file, buf, size);
//	printf(strerror(errno));
	if(ret !=size){
		if(CMD == RX_BLOCK)
			return 1;
		else{
			printf("Failed: %d %d %d\n", ret, size, sizeof(char));
			return 1;
		}
	} else{
		//printf("sent.\n");
		//return 1;
	}

	char ret_buf[8];
	/* Now read back to see if we we're succesful */
	if(CMD!=RX_BLOCK)
		get_response(file, ret_buf);

    return 0;
}

int get_response(int file, char *ret_buf){
	int i =0;
	//printf("Getting response...");
	if(read(file,ret_buf,8) != 8){
	       // printf(strerror(errno));
		printf("unsuccesful read");
		return 1;
        } else {
		//for(i=0; i< 1;i++)
	         //printf("Success! Message = 0x%02x\n",ret_buf[5]);
		printf("Success!\n");
		return 0;
        }

		
}

int jump_to_bsl(int file){
	/* This will vary by implementation on the MSP430 side. This is typically done with a BSLEN/RST GPIO toggle.
	 * In this implementation, a write of 0xFE twice to the MSP will invalidate the CRC at 0x1800  and reboot. 
 	 * When we reboot and fail the CRC check, we will remain in BSL. */


	if (ioctl(file,I2C_SLAVE_FORCE,MSP_ADDR) < 0) {
		printf("Not able to probe MSP address.\n");
		return 1;
        }

	char buf[2];	
	//buf[0] = HEADER;
	buf[0] = 0xFE;
	buf[1] = 0xFE;
	if(write(file, buf, sizeof(buf)) != sizeof(buf)){
		printf("Unable to send message to MSP430 Address, likely already in BSL mode.\n"); 
		return 1;
        } else{
                printf("Jumped to BSL, MSP430 in reset.\n");
		sleep(3);
                return 0;
        }
}

int flash_image(int file, char* firmware_image){
	/* We open a TI-TXT image that has been run through the CRC program. This will eventually do the CRC */
	FILE *fp;
	char data[132];
	char ch, digit;
	char done = 0;
	int i;
	int count = -1;
	char CK[2];
//	printf(firmware_image);
	short bsl_addr = 0;
	short flash_addr = 0;
//	printf("\n");
	fp = fopen(firmware_image,"r");
	int timeout_count = 0;
	int send_result = 0;
	int MAX_TIMEOUTS = 6;
	
	if(!fp){
		printf("Unable to open the input file");
		return 1;
	}

	i = 0;
	printf("Flashing main image...\n");
	while(!done){


	ch = fgetc(fp);

        // The "<<4" gives room for 4 more bits, aka a nibble, aka one hex digit, aka a number within [0,15]
        if(ch>='0' && ch<='9'){
        	digit = (digit<<4) | (ch - '0');
	//	i++;
	}
        else if(ch>='A' && ch<='F'){
	        digit= (digit<<4) | (ch - 'A' + 10);
	//	i++;
	}
        else
            continue; // skip all other (invalid) characters

	if(++i % 2 == 0){
		//printf("digit: %x, %d\n", digit, i);
		count++;
//		printf("count: %d i: %d digit: 0x%X\n", count ,i, digit);	
	                //printf("Checksum: 0x%X, 0x%X, 0x%X", checksum, (checksum & 0x00FF), ((checksum >> 4) & 0x00FF));
		/* The first four three values will be CRC ADDR, CRC, and Flash ADDR -ending in the 14th character */
		if(i < 14){
			switch(count){
				case 0:
					bsl_addr += digit << 8;
					break;
				case 1:
					bsl_addr += digit;
//					printf("bsl_addr: 0x%X", bsl_addr);
					break;
				case 2:
					CK[0] = digit;
					break;
				case 3:
					CK[1] = digit;
//					printf("CK: 0x%X  0x%X", CK[0], CK[1]);
					/* First we'll erase the flash region */
					send_command(file, 0x4, 0x0, ERASE_SEG, bsl_addr & 0xff , (bsl_addr >> 8) & 0xff, 0x0, NULL);
					send_command(file, 0x6, 0x0, RX_BLOCK, bsl_addr & 0xff, (bsl_addr >> 8) & 0xff, 0x0, CK);
					break;
				case 4:
					flash_addr += digit << 8;
					break;
				case 5:
					flash_addr += digit;
					count = -1;
                                        break;		
			}	
		}
		else
			data[count] = digit;
		if(count >= 127){
			
			count = -1;
			send_result = send_command(file, 0x84, 0x0, RX_BLOCK, flash_addr & 0xff, (flash_addr >> 8) & 0xff, 0x0, data);
//			printf("address: %X\n", flash_addr);
			flash_addr+=0x80;
			if(send_result == 0){
				printf(".");
				//flash_addr+=0x80;
			}
			else{

				//while(send_result == 1 && timeout_count < MAX_TIMEOUTS){
					printf("x");
					timeout_count++;
					send_result = send_command(file, 0x84, 0x0, RX_BLOCK, flash_addr & 0xff, (flash_addr >> 8) & 0xff, 0x0, data);
				//}
			}
		}

	digit = 0;
		

	}
	
	//printf("%c\n",ch);
		if(ch == 'q' || timeout_count > MAX_TIMEOUTS)
			done = 1;
	}
	printf("Success!\n");
	fclose(fp);
	return 0;
}

unsigned short get_CRC(int size, char* buf){
	unsigned short checksum = 0xFFFF;
	int i;
	for(i=0; i < size - 5 ; i++)
		checksum = update_crc_ccitt(checksum, buf[3+i]);
	return checksum;

}




