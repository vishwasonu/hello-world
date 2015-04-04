/*
 * Copyright (c) 2007 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 ghhhkll;;l\
 knkl;;\*
 
 /

#include <stdio.h>

#include "xenv_standalone.h"
#include "xparameters.h"
#include "platform.h"

#include "netif/xadapter.h"

#define EMAC_BASEADDR  XPAR_EMACLITE_0_BASEADDR

#ifdef XPAR_CPU_PPC440_CORE_CLOCK_FREQ_HZ
#define USE_UART16550
#endif

int start_application();
int transfer_data();

void
print_ip(char *msg, struct ip_addr *ip) 
{
	print(msg);
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip), 
			ip4_addr3(ip), ip4_addr4(ip));
}

void
print_ip_settings(struct ip_addr *ip, struct ip_addr *mask, struct ip_addr *gw)
{

	print_ip("Board IP: ", ip);
	print_ip("Netmask : ", mask);
	print_ip("Gateway : ", gw);
}


int main()
{
	struct netif *netif, server_netif;
	struct ip_addr ipaddr, netmask, gw;

	/* the mac address of the board. this should be unique per board */
	unsigned char mac_ethernet_address[] = { 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };

#ifdef USE_UART16550
#include "xuartns550_l.h"
#define UART_BAUDRATE 9600                      /* real hardware */
#define UART_BASEADDR XPAR_XPS_UART16550_0_BASEADDR 
#define UART_CLOCK    XPAR_XUARTNS550_CLOCK_HZ 
	XUartNs550_SetBaud(UART_BASEADDR, UART_CLOCK, UART_BAUDRATE);
        XUartNs550_mSetLineControlReg(UART_BASEADDR, XUN_LCR_8_DATA_BITS);
#endif

	netif = &server_netif;

#ifdef __MICROBLAZE__
	microblaze_init_icache_range(0, XPAR_MICROBLAZE_0_CACHE_BYTE_SIZE);
	microblaze_init_dcache_range(0, XPAR_MICROBLAZE_0_DCACHE_BYTE_SIZE);
#endif

	/* enable caches */
	XCACHE_ENABLE_ICACHE();
#ifndef XPAR_CPU_PPC440_CORE_CLOCK_FREQ_HZ
	XCACHE_ENABLE_DCACHE();
#endif

	platform_setup_interrupts();

	/* initliaze IP addresses to be used */
	IP4_ADDR(&ipaddr,  192, 168,   1, 10);
	IP4_ADDR(&netmask, 255, 255, 255,  0);
	IP4_ADDR(&gw,      192, 168,   1,  1);

	//print_app_header();
	print_ip_settings(&ipaddr, &netmask, &gw);

	lwip_init();

  	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(netif, &ipaddr, &netmask, &gw, mac_ethernet_address, EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n\r");
		return -1;
	}
	netif_set_default(netif);
	
	/* Create a new DHCP client for this interface.
	 * Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
	 * the predefined regular intervals after starting the client.
	 */
	/* dhcp_start(netif); */

	/* now enable interrupts */
	platform_enable_interrupts();

	/* specify that the network if is up */
	netif_set_up(netif);

	/* start the application (web server, rxtest, txtest, etc..) */
	start_application();

	/* receive and process packets */
	while (1) {
		xemacif_input(netif);
		transfer_data();
	}
  
	XCACHE_DISABLE_DCACHE();
	XCACHE_DISABLE_ICACHE();

#ifdef __MICROBLAZE__
	microblaze_init_dcache_range(0, XPAR_MICROBLAZE_0_DCACHE_BYTE_SIZE);
	microblaze_init_icache_range(0, XPAR_MICROBLAZE_0_CACHE_BYTE_SIZE);
#endif

	return 0;
}
