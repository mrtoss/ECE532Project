/*
 * Copyright (C) 2009 - 2018 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>
#include "xparameters.h"

#include "lwip/err.h"
#include "lwip/tcp.h"
#if defined (__arm__) || defined (__aarch64__)
#include "xil_printf.h"
#endif


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

char* process_string(char* payload, int* payload_len);
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

int transfer_data() {
	return 0;
}

void print_app_header()
{
#if (LWIP_IPV6==0)
	xil_printf("\n\r\n\r-----lwIP TCP echo server ------\n\r");
#else
	xil_printf("\n\r\n\r-----lwIPv6 TCP echo server ------\n\r");
#endif
	xil_printf("TCP packets sent to port 6001 will be echoed back\n\r");
}

err_t recv_callback(void *arg, struct tcp_pcb *tpcb,
                               struct pbuf *p, err_t err)
{
	/* do not read the packet if we are not in ESTABLISHED state */
	if (!p) {
		tcp_close(tpcb);
		tcp_recv(tpcb, NULL);
		return ERR_OK;
	}

	/* indicate that the packet has been received */
	tcp_recved(tpcb, p->len);

	/* echo back the payload */
	/* in this case, we assume that the payload is < TCP_SND_BUF */
	if (tcp_sndbuf(tpcb) > p->len) {
		char* payload_copy = malloc(p->len);
		int payload_len = p->len;
		strcpy(payload_copy, (char*) p->payload);
		char* result_str = process_string(payload_copy, &payload_len);
		print("After return\n");

		int try_len = 5;
		try_len = payload_len;
		err = tcp_write(tpcb, result_str, try_len, 1);
		free(payload_copy);
		free(result_str);
	} else
		xil_printf("no space in tcp_sndbuf\n\r");

	/* free the received pbuf */
	pbuf_free(p);

	return ERR_OK;
}

err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	static int connection = 1;

	/* set the receive callback for this connection */
	tcp_recv(newpcb, recv_callback);

	/* just use an integer number indicating the connection id as the
	   callback argument */
	tcp_arg(newpcb, (void*)(UINTPTR)connection);

	/* increment for subsequent accepted connections */
	connection++;

	return ERR_OK;
}


int start_application()
{
	struct tcp_pcb *pcb;
	err_t err;
	unsigned port = 7;

	/* create new TCP PCB structure */
	pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (!pcb) {
		xil_printf("Error creating PCB. Out of Memory\n\r");
		return -1;
	}

	/* bind to specified @port */
	err = tcp_bind(pcb, IP_ANY_TYPE, port);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n\r", port, err);
		return -2;
	}

	/* we do not need any arguments to callback functions */
	tcp_arg(pcb, NULL);

	/* listen for connections */
	pcb = tcp_listen(pcb);
	if (!pcb) {
		xil_printf("Out of memory while tcp_listen\n\r");
		return -3;
	}

	/* specify callback to use for incoming connections */
	tcp_accept(pcb, accept_callback);

	xil_printf("TCP echo server started @ port %d\n\r", port);

	return 0;
}


// If the incoming string start with "enc:" The content is encrypted and returned
// If the incoming string start with "dec:" The content is decrypted and returned
// Else, the program assumes the incoming string is encrypted, and decrypts the incoming string
// If the decryption result start with "hello", the server will respond an encrypted version of "hi"
char* process_string(char* payload, int* payload_len){
	print("The payload is: ");
	for (int i=0; i<*payload_len; i++){
		xil_printf("%c", payload[i]);
	}
	print("\n");
	xil_printf("The length is %d\n", *payload_len);
	char enc_identifier[5] = "enc:";
	char dec_identifier[5] = "dec:";
	// if a string is to be encrypted, it starts with "enc:"
	// if a string is to be encrypted, it starts with "dec:"
	int enc_compare = strncmp(enc_identifier, payload, 4);
	int dec_compare = strncmp(dec_identifier, payload, 4);
	if (!enc_compare){
		print ("doing encryption\n");
		char* to_be_enc = (char*) malloc(32);
		for (int i=0; i<32; i++){
			to_be_enc[i] = '\x00';
		}
		for (int i = 4; i<*payload_len; i++){
			to_be_enc[i-4] = payload[i];
		}
		encryption(to_be_enc);
		*payload_len = 32;
		free(to_be_enc);
		return get_result_str();

	} else if (!dec_compare){
		print ("doing decryption\n");
		char* to_be_dec = (char*) malloc(32);
		for (int i=0; i<32; i++){
			to_be_dec[i] = '\x00';
		}
		for (int i = 4; i<*payload_len; i++){
			to_be_dec[i-4] = payload[i];
		}
		print("Before decryption\n");
		decryption(to_be_dec);
		print("Decryption done\n");
		char* dec_result = get_result_str();
		char* plain_str = remove_padding(dec_result);
		xil_printf("Decryption result is: ");
		for(int i = 0; i< 32; i++){
			xil_printf("%c", plain_str[i]);
		}
		xil_printf("\n");
		*payload_len = 32;
		free(to_be_dec);
		return plain_str;
	} else{
		//normal case, decrypt the incoming string and return an encrypted string
		char* payload_copy = (char*)malloc(32);
		for (int i=0; i<32; i++){
			payload_copy[i] = payload[i];
		}
		decryption(payload);
		char* dec_result = get_result_str();
		char* plain_str = remove_padding(dec_result);
		xil_printf("Decryption result is: ");
		for(int i = 0; i< 32; i++){
			xil_printf("%c", plain_str[i]);
		}
		xil_printf("\n");

		char hello_str[] = "hello";
		char goodbye_str[] = "goodbye";
		int hello_compare = strncmp(hello_str, plain_str, 5);
		int goodbye_compare = strncmp(goodbye_str, plain_str, 7);

		if (!hello_compare){
			print("hello received hello\n");
			char* tobeenc = (char*) malloc(32);
			tobeenc = "hi\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
			*payload_len = 32;
			encryption(tobeenc);
			char* ret_str = get_result_str();
			free(payload_copy);
			free(dec_result);
			free(plain_str);
			return ret_str;
		} else if (!goodbye_compare){
			print("goodbye received goodbye\n");
			char tobeenc[32] = "bye";
			encryption(tobeenc);
			*payload_len = 32;
			char* ret_str = get_result_str();
			free(payload_copy);
			free(dec_result);
			free(plain_str);
			return ret_str;
		} else {
			print("no response available\n");
			char tobeenc[32] = "invalid message";
			encryption(tobeenc);
			char* ret_str = get_result_str();
			*payload_len = 32;
			free(payload_copy);
			free(dec_result);
			free(plain_str);
			return ret_str;
		}
		//return payload;
	}
}

void encryption(char* s){
	char* new_str = add_padding(s);
	char* reverse_s = reverse_string(new_str);
	print("Before do_enc\n");
	do_enc(reverse_s);
	print("After do_enc\n");
	free(new_str);
	free(reverse_s);
	print_enc_result();
}

void decryption(char* s){
	char* reverse_s = reverse_string(s);
	print("Before do_dec\n");
	do_dec(reverse_s);
	print("After do_dec\n");
	free(reverse_s);
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
	free(final_str);
}





