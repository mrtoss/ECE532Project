/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"

volatile unsigned int* gpio0_membase = (unsigned int*) XPAR_AXI_GPIO_0_BASEADDR;
volatile unsigned int* gpio1_membase = (unsigned int*) XPAR_AXI_GPIO_1_BASEADDR;
volatile unsigned int* gpio2_membase = (unsigned int*) XPAR_AXI_GPIO_2_BASEADDR;
volatile unsigned int* gpio3_membase = (unsigned int*) XPAR_AXI_GPIO_3_BASEADDR;
volatile unsigned int* gpio4_membase = (unsigned int*) XPAR_AXI_GPIO_4_BASEADDR;
volatile unsigned int* gpio5_membase = (unsigned int*) XPAR_AXI_GPIO_5_BASEADDR;
volatile unsigned int* gpio6_membase = (unsigned int*) XPAR_AXI_GPIO_6_BASEADDR;
volatile unsigned int* gpio7_membase = (unsigned int*) XPAR_AXI_GPIO_7_BASEADDR;
volatile unsigned int* gpio8_membase = (unsigned int*) XPAR_AXI_GPIO_8_BASEADDR;
volatile unsigned int* gpio9_membase = (unsigned int*) XPAR_AXI_GPIO_9_BASEADDR;
volatile unsigned int* gpio10_membase = (unsigned int*) XPAR_AXI_GPIO_10_BASEADDR;
volatile unsigned int* gpio11_membase = (unsigned int*) XPAR_AXI_GPIO_11_BASEADDR;
volatile unsigned int* gpio12_membase = (unsigned int*) XPAR_AXI_GPIO_12_BASEADDR;
volatile unsigned int* gpio13_membase = (unsigned int*) XPAR_AXI_GPIO_13_BASEADDR;
volatile unsigned int* gpio14_membase = (unsigned int*) XPAR_AXI_GPIO_14_BASEADDR;
volatile unsigned int* gpio15_membase = (unsigned int*) XPAR_AXI_GPIO_15_BASEADDR;
volatile unsigned int* gpio16_membase = (unsigned int*) XPAR_AXI_GPIO_16_BASEADDR;

char* fuckyoubitch(char* jiba);
char* add_padding(char* s);
char* remove_padding(char* s);
void print_enc_result();
void print_dec_result();
char* reverse_string(char* in_s);
void do_enc(char* s);
void do_dec(char* s);
void encryption(char* s);
void decryption(char* s);
char* get_result_str();


int main()
{
    init_platform();

    print("Hello World\n\r");
    int seed;
    print("Enter random seed:\n");
    scanf("%d",&seed);

    /* Intializes random number generator */
    srand(seed);

    char to_be_enc[32] = "goodbye";
    xil_printf("To be encrypted string: \n");
    for(int i = 0; i< 32; i++){
    	xil_printf("%c", to_be_enc[i]);
    }
    xil_printf("\n");
    encryption(to_be_enc);

    char* abc = get_result_str();
    print("\nTo be decrypted string: \n");
    for (int i = 0; i < 32; i++){
    	xil_printf("%c", *(abc+i));
    }
    print("\n");
    decryption(abc);

    cleanup_platform();
    return 0;
}

char* fuckyoubitch(char* jiba){
	xil_printf("address of s: %08X\n", (unsigned int) jiba);
	return 0;
}

void encryption(char* s){
	char* new_str = add_padding(s);
	char* reverse_s = reverse_string(new_str);
	do_enc(reverse_s);
	print_enc_result();
}

void decryption(char* s){
	char* reverse_s = reverse_string(s);
	do_dec(reverse_s);
	//print_dec_result();
	char* dec_result = get_result_str();
	char* plain_str = remove_padding(dec_result);
	xil_printf("Decryption result is: ");
	for(int i = 0; i< 32; i++){
		xil_printf("%c", plain_str[i]);
	}
	xil_printf("\n");
}

char* add_padding(char* s){
	int char_count = 0;
	for (int i=0; i<32; i++){
		if (char_count > 21){
			// the when the string is longer than 21, cant add padding
			return s;
		}
		else if (s[i] == 0){
			break;
		} else {
			char_count++;
		}
	}
	char* padded_string = (char*) malloc(32);
	padded_string[0] = '\x00';
	padded_string[1] = '\x02';
	padded_string[32-char_count-1] = '\x00';

	for (int k=2; k < 32-char_count-1; k++){
		padded_string[k] =  1 + rand() % 255;
	}

	for (int j=0; j<char_count; j++){
		padded_string[32-char_count+j] = s[j];
	}
	return padded_string;
}

char* remove_padding(char* s){
	char* plain_string = (char*) malloc(32);
	int found_zero = 0;
	int plain_str_counter = 0;

		for (int i=0; i<32; i++){
			plain_string[i] = '\x00';
		}

	for (int j=2; j<32; j++){
		char curr_char = *(s+j);
		if (found_zero){
			plain_string[plain_str_counter] = s[j];
			plain_str_counter++;
		}
		if (curr_char == '\x00'){
			found_zero = 1;
		}
	}
	return plain_string;
}

void do_enc(char* s){
	// write message
	unsigned int* write_ptr = (unsigned int*) s;
	unsigned int val = *write_ptr;
	gpio0_membase[0] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio0_membase[2] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio1_membase[0] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio1_membase[2] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio2_membase[0] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio2_membase[2] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio3_membase[0] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio3_membase[2] = val;

	// write key (public exponent)
	gpio4_membase[0] = 0x00010001;
	gpio4_membase[2] = 0x00000000;
	gpio5_membase[0] = 0x00000000;
	gpio5_membase[2] = 0x00000000;
	gpio6_membase[0] = 0x00000000;
	gpio6_membase[2] = 0x00000000;
	gpio7_membase[0] = 0x00000000;
	gpio7_membase[2] = 0x00000000;

	// write modulus
	gpio8_membase[0] = 0xd43055eb;
	gpio8_membase[2] = 0xb98f1e6c;
	gpio9_membase[0] = 0x172e6b43;
	gpio9_membase[2] = 0x23172b4f;
	gpio10_membase[0] = 0xc24c7b58;
	gpio10_membase[2] = 0x524b5a88;
	gpio11_membase[0] = 0xea35c996;
	gpio11_membase[2] = 0xc3d6283a;

	// start encryption
	gpio16_membase[0] = 1;
	gpio16_membase[0] = 0;

	val = gpio16_membase[2];
	while (val == 0){
	    val = gpio16_membase[2];
	}
	val = gpio12_membase[2];
	while (val == 0){
	    val = gpio12_membase[2];
	}

}

void do_dec(char* s){
	// write message
	unsigned int* write_ptr = (unsigned int*) s;
	unsigned int val = *write_ptr;
	gpio0_membase[0] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio0_membase[2] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio1_membase[0] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio1_membase[2] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio2_membase[0] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio2_membase[2] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio3_membase[0] = val;
	write_ptr = write_ptr+1;
	val = *write_ptr;
	gpio3_membase[2] = val;

	// write key (private exponent)
	gpio4_membase[0] = 0xe3c06c89;
	gpio4_membase[2] = 0xe2c7376b;
	gpio5_membase[0] = 0x90912532;
	gpio5_membase[2] = 0x8551ceef;
	gpio6_membase[0] = 0xe26481da;
	gpio6_membase[2] = 0xf624ed5b;
	gpio7_membase[0] = 0xed5b4b69;
	gpio7_membase[2] = 0x77d8d8b7;

	// write modulus
	gpio8_membase[0] = 0xd43055eb;
	gpio8_membase[2] = 0xb98f1e6c;
	gpio9_membase[0] = 0x172e6b43;
	gpio9_membase[2] = 0x23172b4f;
	gpio10_membase[0] = 0xc24c7b58;
	gpio10_membase[2] = 0x524b5a88;
	gpio11_membase[0] = 0xea35c996;
	gpio11_membase[2] = 0xc3d6283a;

	// start encryption
	gpio16_membase[0] = 1;
	gpio16_membase[0] = 0;

	val = gpio16_membase[2];
	while (val == 0){
	    val = gpio16_membase[2];
	}
	val = gpio12_membase[2];
	while (val == 0){
	    val = gpio12_membase[2];
	}

}

char* reverse_string(char* in_s){
	char* reverse_s = (char*) malloc(32);
	for (int i=0; i<32; i++){
		reverse_s[i] = in_s[31-i];
	}
	return reverse_s;
}

char* get_result_str(){
	char* final_str = (char*)malloc(33);
	char* final_copy = final_str;

	char* read_ptr = (char*)(gpio12_membase+2);
	for (int i=3; i>=0; i--){
		*(final_copy+(3-i)) = *(read_ptr+i);
	}
	final_copy = final_copy + 4;

	read_ptr = (char*)(gpio12_membase);
	for (int i=3; i>=0; i--){
		*(final_copy+(3-i)) = *(read_ptr+i);
	}
	final_copy = final_copy + 4;

	read_ptr = (char*)(gpio13_membase+2);
	for (int i=3; i>=0; i--){
		*(final_copy+(3-i)) = *(read_ptr+i);
	}
	final_copy = final_copy + 4;

	read_ptr = (char*)(gpio13_membase);
	for (int i=3; i>=0; i--){
		*(final_copy+(3-i)) = *(read_ptr+i);
	}
	final_copy = final_copy + 4;

	read_ptr = (char*)(gpio14_membase+2);
	for (int i=3; i>=0; i--){
		*(final_copy+(3-i)) = *(read_ptr+i);
	}
	final_copy = final_copy + 4;

	read_ptr = (char*)(gpio14_membase);
	for (int i=3; i>=0; i--){
		*(final_copy+(3-i)) = *(read_ptr+i);
	}
	final_copy = final_copy + 4;

	read_ptr = (char*)(gpio15_membase+2);
	for (int i=3; i>=0; i--){
		*(final_copy+(3-i)) = *(read_ptr+i);
	}
	final_copy = final_copy + 4;

	read_ptr = (char*)(gpio15_membase);
	for (int i=3; i>=0; i--){
		*(final_copy+(3-i)) = *(read_ptr+i);
	}

	final_str[32] = 0;
	return final_str;
}

void print_enc_result(){
	print("Encryption result is: ");
	xil_printf("%08X%08X%08X%08X%08X%08X%08X%08X", gpio12_membase[2], gpio12_membase[0],
			gpio13_membase[2], gpio13_membase[0],
			gpio14_membase[2], gpio14_membase[0],
			gpio15_membase[2], gpio15_membase[0]);
	print("\n");
}

void print_dec_result(){
	char* final_str = get_result_str();
	xil_printf("Decryption result is: ");
	for(int i = 0; i< 32; i++){
		xil_printf("%c", final_str[i]);
	}
	xil_printf("\n");
}


