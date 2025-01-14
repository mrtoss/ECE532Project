/******************************************************************************
*
* Copyright (C) 2009 - 2017 Xilinx, Inc.  All rights reserved.
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

//Standard library includes
#include <stdio.h>
#include <string.h>

//BSP includes for peripherals
#include "xparameters.h"
#include "netif/xadapter.h"

#include "platform.h"
#include "platform_config.h"
#if defined (__arm__) || defined(__aarch64__)
#include "xil_printf.h"
#endif
#include "xil_cache.h"

//LWIP include files
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "lwip/inet.h"
#include "lwip/etharp.h"
#if LWIP_IPV6==1
#include "lwip/ip.h"
#else
#if LWIP_DHCP==1
#include "lwip/dhcp.h"
#endif
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

void lwip_init(); /* missing declaration in lwIP */
struct netif *echo_netif;

//TCP Network Params
#define SRC_MAC_ADDR {0x00, 0x0a, 0x35, 0x00, 0x01, 0x07}
#define SRC_IP4_ADDR "1.1.7.2"
#define IP4_NETMASK "255.255.255.0"
#define IP4_GATEWAY "1.1.0.1"
#define SRC_PORT 7

#define DEST_IP4_ADDR  "1.1.5.2"
#define DEST_IP6_ADDR "fe80::6600:6aff:fe71:fde6"
#define DEST_PORT 7

#define TCP_SEND_BUFSIZE 32

//Function prototypes
#if LWIP_IPV6==1
void print_ip6(char *msg, ip_addr_t *ip);
#else
void print_ip(char *msg, ip_addr_t *ip);
void print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw);
#endif
int setup_client_conn();
void tcp_fasttmr(void);
void tcp_slowtmr(void);

//Function prototypes for callbacks
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void tcp_client_err(void *arg, err_t err);
static void tcp_client_close(struct tcp_pcb *pcb);

//DHCP global variables
#if LWIP_IPV6==0
#if LWIP_DHCP==1
extern volatile int dhcp_timoutcntr;
err_t dhcp_start(struct netif *netif);
#endif
#endif

//Networking global variables
extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;
static struct netif server_netif;
struct netif *app_netif;
static struct tcp_pcb *c_pcb;
char is_connected;


int main()
{
	//Varibales for IP parameters
#if LWIP_IPV6==0
	ip_addr_t ipaddr, netmask, gw;
#endif

	//The mac address of the board. this should be unique per board
	unsigned char mac_ethernet_address[] = SRC_MAC_ADDR;

	//Network interface
	app_netif = &server_netif;

	//Initialize platform
	init_platform();

	//Defualt IP parameter values
#if LWIP_IPV6==0
#if LWIP_DHCP==1
    ipaddr.addr = 0;
	gw.addr = 0;
	netmask.addr = 0;
#else
	(void)inet_aton(SRC_IP4_ADDR, &ipaddr);
	(void)inet_aton(IP4_NETMASK, &netmask);
	(void)inet_aton(IP4_GATEWAY, &gw);
#endif
#endif

	//LWIP initialization
	lwip_init();

	//Setup Network interface and add to netif_list
#if (LWIP_IPV6 == 0)
	if (!xemac_add(app_netif, &ipaddr, &netmask,
						&gw, mac_ethernet_address,
						PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n");
		return -1;
	}
#else
	if (!xemac_add(app_netif, NULL, NULL, NULL, mac_ethernet_address,
						PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n");
		return -1;
	}
	app_netif->ip6_autoconfig_enabled = 1;

	netif_create_ip6_linklocal_address(app_netif, 1);
	netif_ip6_addr_set_state(app_netif, 0, IP6_ADDR_VALID);

#endif
	netif_set_default(app_netif);

	//Now enable interrupts
	platform_enable_interrupts();

	//Specify that the network is up
	netif_set_up(app_netif);

#if (LWIP_IPV6 == 0)
#if (LWIP_DHCP==1)
	/* Create a new DHCP client for this interface.
	 * Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
	 * the predefined regular intervals after starting the client.
	 */
	dhcp_start(app_netif);
	dhcp_timoutcntr = 24;

	while(((app_netif->ip_addr.addr) == 0) && (dhcp_timoutcntr > 0))
		xemacif_input(app_netif);

	if (dhcp_timoutcntr <= 0) {
		if ((app_netif->ip_addr.addr) == 0) {
			xil_printf("DHCP Timeout\n");
			xil_printf("Configuring default IP of %s\n", SRC_IP4_ADDR);
			(void)inet_aton(SRC_IP4_ADDR, &(app_netif->ip_addr));
			(void)inet_aton(IP4_NETMASK, &(app_netif->netmask));
			(void)inet_aton(IP4_GATEWAY, &(app_netif->gw));
		}
	}

	ipaddr.addr = app_netif->ip_addr.addr;
	gw.addr = app_netif->gw.addr;
	netmask.addr = app_netif->netmask.addr;
#endif
#endif

	//Print connection settings
#if (LWIP_IPV6 == 0)
	print_ip_settings(&ipaddr, &netmask, &gw);
#else
	print_ip6("Board IPv6 address ", &app_netif->ip6_addr[0].u_addr.ip6);
#endif

	//Gratuitous ARP to announce MAC/IP address to network
	etharp_gratuitous(app_netif);

	//Setup connection
	setup_client_conn();

	//Event loop
	while (1) {
		//Call tcp_tmr functions
		//Must be called regularly
		if (TcpFastTmrFlag) {
			tcp_fasttmr();
			TcpFastTmrFlag = 0;
		}
		if (TcpSlowTmrFlag) {
			tcp_slowtmr();
			TcpSlowTmrFlag = 0;
		}

		//Process data queued after interupt
		xemacif_input(app_netif);

		//setup_client_conn();



		//ADD CODE HERE to be repeated constantly
		// Note - should be non-blocking
		// Note - can check is_connected global var to see if connection open
		//Just send a single packet

		//Set callback values & functions

		//END OF ADDED CODE


	}

	//Never reached
	cleanup_platform();

	return 0;
}


#if LWIP_IPV6==1
void print_ip6(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf(" %x:%x:%x:%x:%x:%x:%x:%x\n",
			IP6_ADDR_BLOCK1(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK2(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK3(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK4(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK5(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK6(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK7(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK8(&ip->u_addr.ip6));

}
#else
void print_ip(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\n", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}

void print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{

	print_ip("Board IP: ", ip);
	print_ip("Netmask : ", mask);
	print_ip("Gateway : ", gw);
}
#endif


int setup_client_conn()
{
	struct tcp_pcb *pcb;
	err_t err;
	ip_addr_t remote_addr;

	xil_printf("Setting up client connection\n");

#if LWIP_IPV6==1
	remote_addr.type = IPADDR_TYPE_V6;
	err = inet6_aton(DEST_IP6_ADDR, &remote_addr);
#else
	err = inet_aton(DEST_IP4_ADDR, &remote_addr);
#endif

	if (!err) {
		xil_printf("Invalid Server IP address: %d\n", err);
		return -1;
	}

	//Create new TCP PCB structure
	pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (!pcb) {
		xil_printf("Error creating PCB. Out of Memory\n");
		return -1;
	}

	//Bind to specified @port
	err = tcp_bind(pcb, IP_ANY_TYPE, SRC_PORT);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n", SRC_PORT, err);
		return -2;
	}

	//Connect to remote server (with callback on connection established)
	err = tcp_connect(pcb, &remote_addr, DEST_PORT, tcp_client_connected);
	if (err) {
		xil_printf("Error on tcp_connect: %d\n", err);
		tcp_client_close(pcb);
		return -1;
	}

	is_connected = 0;

	xil_printf("Waiting for server to accept connection\n");

	return 0;
}

static void tcp_client_close(struct tcp_pcb *pcb)
{
	err_t err;

	xil_printf("Closing Client Connection\n");

	if (pcb != NULL) {
		tcp_sent(pcb, NULL);
		tcp_recv(pcb,NULL);
		tcp_err(pcb, NULL);
		err = tcp_close(pcb);
		if (err != ERR_OK) {
			/* Free memory with abort */
			tcp_abort(pcb);
		}
	}
}


// client asks for user input, encrypt the input
// sends the encrypted input to the server,
// receive the reply from the server,
// decrypt the reply
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	if (err != ERR_OK) {
		tcp_client_close(tpcb);
		xil_printf("Connection error\n");
		return err;
	}

	xil_printf("Connection to server established\n");

	//Store state (for callbacks)
	c_pcb = tpcb;
	is_connected = 1;

	//Set callback values & functions
	tcp_arg(c_pcb, NULL);
	tcp_recv(c_pcb, tcp_client_recv);
	tcp_sent(c_pcb, tcp_client_sent);
	tcp_err(c_pcb, tcp_client_err);



	//ADD CODE HERE to do when connection established

	//Just send a single packet
	u8_t apiflags = TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE;
	char send_buf[TCP_SEND_BUFSIZE];
	u32_t i;
	int seed = 50;
	//print("Enter random seed:\n");
	//scanf("%d",&seed);
	srand(seed);

	for(i = 0; i < TCP_SEND_BUFSIZE-1; i = i + 1)
	{
		send_buf[i] = 0;
	}
	send_buf[TCP_SEND_BUFSIZE-1] = '\n';
	print("Enter string to be encrypted:\n");
	scanf("%[^\r]%*c", send_buf);
	for (int i = 0; i < TCP_SEND_BUFSIZE; i++){
		xil_printf("%c", send_buf[i]);
	}
	print("\n");
	xil_printf("The string is: %s\n", send_buf);

	encryption(send_buf);
	char* abc = get_result_str();

	//Loop until enough room in buffer (should be right away)
	while (tcp_sndbuf(c_pcb) < TCP_SEND_BUFSIZE);

	//Enqueue some data to send
	err = tcp_write(c_pcb, abc, TCP_SEND_BUFSIZE, apiflags);
	if (err != ERR_OK) {
		xil_printf("TCP client: Error on tcp_write: %d\n", err);
		return err;
	}

	//send the data packet
	err = tcp_output(c_pcb);
	if (err != ERR_OK) {
		xil_printf("TCP client: Error on tcp_output: %d\n",err);
		return err;
	}

	xil_printf("Packet data sent\n");

	//END OF ADDED CODE



	return ERR_OK;
}

static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	//If no data, connection closed
	if (!p) {
		xil_printf("No data received\n");
		tcp_client_close(tpcb);
		return ERR_OK;
	}



	//ADD CODE HERE to do on packet reception

	//Print message
	xil_printf("Packet received, %d bytes\n", p->tot_len);

	//Print packet contents to terminal
	char* packet_data = (char*) malloc(p->tot_len);
	pbuf_copy_partial(p, packet_data, p->tot_len, 0); //Note - inefficient way to access packet data
	u32_t i;

//	for(i = 0; i < p->tot_len; i = i + 1)
//		xil_printf("%02X", (unsigned char) packet_data[i]);
		//putchar(packet_data[i]);
	for (i = 0; i <  p->tot_len; i = i + 1){
		xil_printf("%02X", *((unsigned char*) (packet_data + i)) );
	}
	print("\n");
	decryption(packet_data);

	//END OF ADDED CODE




	//Indicate done processing
	tcp_recved(tpcb, p->tot_len);

	//Free the received pbuf
	pbuf_free(p);

	tcp_client_close(tpcb);
	return 0;
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{


	//ADD CODE HERE to do on packet acknowledged

	//Print message
	xil_printf("Packet sent successfully, %d bytes\n", len);

	//END OF ADDED CODE



	return 0;
}

static void tcp_client_err(void *arg, err_t err)
{
	LWIP_UNUSED_ARG(err);
	tcp_client_close(c_pcb);
	c_pcb = NULL;
	xil_printf("TCP connection aborted\n");
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

//void decryption(char* s){
//	char* reverse_s = reverse_string(s);
//	print("Before do_dec\n");
//	do_dec(reverse_s);
//	print("After do_dec\n");
//	free(reverse_s);
//}

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
	free(reverse_s);
	free(dec_result);
	free(plain_str);
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



