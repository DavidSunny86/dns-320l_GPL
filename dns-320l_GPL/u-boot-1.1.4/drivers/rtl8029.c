/*
 * Realtek 8029AS Ethernet
 * (C) Copyright 2002-2003
 * Xue Ligong(lgxue@hotmail.com),Wang Kehao, ESLAB, whut.edu.cn
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * This code works in 8bit mode.
 * If you need to work in 16bit mode, PLS change it!
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include "rtl8029.h"
#include <net.h>
#include "pci.h"

#ifdef CONFIG_DRIVER_RTL8029

#if (CONFIG_COMMANDS & CFG_CMD_NET)

struct eth_device *rtl8029_dev;

/* packet page register access functions */


static unsigned char get_reg (unsigned int regno)
{
	return (*(volatile unsigned char *) regno);
}


static void put_reg (unsigned int regno, unsigned char val)
{
	*(volatile unsigned char *) regno = val;
}

static void eth_reset (void)
{
	unsigned char ucTemp;

	/* reset NIC */
	ucTemp = get_reg (RTL8029_RESET);
	put_reg (RTL8029_RESET, ucTemp);
	put_reg (RTL8029_INTERRUPTSTATUS, 0xff);
	udelay (2000);		/* wait for 2ms */
}

void rtl8029_get_enetaddr (uchar * addr)
{
	unsigned char i;
	unsigned char temp;

	eth_reset ();

	put_reg (RTL8029_COMMAND, RTL8029_REMOTEDMARD);
	put_reg (RTL8029_DATACONFIGURATION, 0x48);
	put_reg (RTL8029_REMOTESTARTADDRESS0, 0x00);
	put_reg (RTL8029_REMOTESTARTADDRESS1, 0x00);
	put_reg (RTL8029_REMOTEBYTECOUNT0, 12);
	put_reg (RTL8029_REMOTEBYTECOUNT1, 0x00);
	put_reg (RTL8029_COMMAND, RTL8029_REMOTEDMARD);
	printf ("MAC: ");
	for (i = 0; i < 6; i++) {
		temp = get_reg (RTL8029_DMA_DATA);
		*addr++ = temp;
		temp = get_reg (RTL8029_DMA_DATA);
		printf ("%x:", temp);
	}

	while ((!get_reg (RTL8029_INTERRUPTSTATUS) & 0x40));
	printf ("\b \n");
	put_reg (RTL8029_REMOTEBYTECOUNT0, 0x00);
	put_reg (RTL8029_REMOTEBYTECOUNT1, 0x00);
	put_reg (RTL8029_COMMAND, RTL8029_PAGE0);
}


void rtl8029_eth_halt (struct eth_device *dev)
{
	put_reg (RTL8029_COMMAND, 0x01);
}

int rtl8029_eth_init (struct eth_device *dev,bd_t * bd)
{
	uchar *mac = (uchar *)dev->enetaddr;

	rtl8029_get_enetaddr(mac);

	eth_reset ();
	put_reg (RTL8029_COMMAND, RTL8029_PAGE0STOP);
	put_reg (RTL8029_DATACONFIGURATION, 0x48);
	put_reg (RTL8029_REMOTEBYTECOUNT0, 0x00);
	put_reg (RTL8029_REMOTEBYTECOUNT1, 0x00);
	put_reg (RTL8029_RECEIVECONFIGURATION, 0x00);	/*00; */
	put_reg (RTL8029_TRANSMITPAGE, RTL8029_TPSTART);
	put_reg (RTL8029_TRANSMITCONFIGURATION, 0x02);
	put_reg (RTL8029_PAGESTART, RTL8029_PSTART);
	put_reg (RTL8029_BOUNDARY, RTL8029_PSTART);
	put_reg (RTL8029_PAGESTOP, RTL8029_PSTOP);
	put_reg (RTL8029_INTERRUPTSTATUS, 0xff);
	put_reg (RTL8029_INTERRUPTMASK, 0x11);	/*b; */
	put_reg (RTL8029_COMMAND, RTL8029_PAGE1STOP);
	put_reg (RTL8029_PHYSICALADDRESS0, dev->enetaddr[0]);
	put_reg (RTL8029_PHYSICALADDRESS1, dev->enetaddr[1]);
	put_reg (RTL8029_PHYSICALADDRESS2, dev->enetaddr[2]);
	put_reg (RTL8029_PHYSICALADDRESS3, dev->enetaddr[3]);
	put_reg (RTL8029_PHYSICALADDRESS4, dev->enetaddr[4]);
	put_reg (RTL8029_PHYSICALADDRESS5, dev->enetaddr[5]);
	put_reg (RTL8029_MULTIADDRESS0, 0x00);
	put_reg (RTL8029_MULTIADDRESS1, 0x00);
	put_reg (RTL8029_MULTIADDRESS2, 0x00);
	put_reg (RTL8029_MULTIADDRESS3, 0x00);
	put_reg (RTL8029_MULTIADDRESS4, 0x00);
	put_reg (RTL8029_MULTIADDRESS5, 0x00);
	put_reg (RTL8029_MULTIADDRESS6, 0x00);
	put_reg (RTL8029_MULTIADDRESS7, 0x00);
	put_reg (RTL8029_CURRENT, RTL8029_PSTART);
	put_reg (RTL8029_COMMAND, RTL8029_PAGE0);
	put_reg (RTL8029_TRANSMITCONFIGURATION, 0xe0);	/*58; */



	return 1;
}


static unsigned char nic_to_pc (void)
{
	unsigned char rec_head_status;
	unsigned char next_packet_pointer;
	unsigned char packet_length0;
	unsigned char packet_length1;
	unsigned short rxlen = 0;
	unsigned int i = 4;
	unsigned char current_point;
	unsigned char *addr;

	/*
	 * The RTL8029's first 4B is packet status,page of next packet
	 * and packet length(2B).So we receive the fist 4B.
	 */
	put_reg (RTL8029_REMOTESTARTADDRESS1, get_reg (RTL8029_BOUNDARY));
	put_reg (RTL8029_REMOTESTARTADDRESS0, 0x00);
	put_reg (RTL8029_REMOTEBYTECOUNT1, 0x00);
	put_reg (RTL8029_REMOTEBYTECOUNT0, 0x04);



	put_reg (RTL8029_COMMAND, RTL8029_REMOTEDMARD);

	rec_head_status = get_reg (RTL8029_DMA_DATA);
	next_packet_pointer = get_reg (RTL8029_DMA_DATA);
	packet_length0 = get_reg (RTL8029_DMA_DATA);
	packet_length1 = get_reg (RTL8029_DMA_DATA);

	put_reg (RTL8029_COMMAND, RTL8029_PAGE0);
	/*Packet length is in two 8bit registers */
	rxlen = packet_length1;
	rxlen = (((rxlen << 8) & 0xff00) + packet_length0);
	rxlen -= 4;


	if (rxlen > PKTSIZE_ALIGN + PKTALIGN)
		printf ("packet too big!\n");

	/*Receive the packet */
	put_reg (RTL8029_REMOTESTARTADDRESS0, 0x04);
	put_reg (RTL8029_REMOTESTARTADDRESS1, get_reg (RTL8029_BOUNDARY));

	put_reg (RTL8029_REMOTEBYTECOUNT0, (rxlen & 0xff));
	put_reg (RTL8029_REMOTEBYTECOUNT1, ((rxlen >> 8) & 0xff));


	put_reg (RTL8029_COMMAND, RTL8029_REMOTEDMARD);

	for (addr = (unsigned char *) NetRxPackets[0], i = rxlen; i > 0; i--)
		{

			*addr++ = get_reg (RTL8029_DMA_DATA);
		}
	/* Pass the packet up to the protocol layers. */
	NetReceive (NetRxPackets[0], rxlen);

	while (!(get_reg (RTL8029_INTERRUPTSTATUS)) & 0x40);	/* wait for the op. */

	/*
	 * To test whether the packets are all received,get the
	 * location of current point
	 */
	put_reg (RTL8029_COMMAND, RTL8029_PAGE1);
	current_point = get_reg (RTL8029_CURRENT);
	put_reg (RTL8029_COMMAND, RTL8029_PAGE0);
	put_reg (RTL8029_BOUNDARY, next_packet_pointer);
	return current_point;
}

/* Get a data block via Ethernet */
extern int rtl8029_eth_rx (struct eth_device *dev)
{
	unsigned char temp, current_point;

	put_reg (RTL8029_COMMAND, RTL8029_PAGE0);


	while (1) {
		temp = get_reg (RTL8029_INTERRUPTSTATUS);


		if (temp & 0x90) {
			/*overflow */
			put_reg (RTL8029_COMMAND, RTL8029_PAGE0STOP);
			udelay (2000);
			put_reg (RTL8029_REMOTEBYTECOUNT0, 0);
			put_reg (RTL8029_REMOTEBYTECOUNT1, 0);
			put_reg (RTL8029_TRANSMITCONFIGURATION, 2);
			do {
				current_point = nic_to_pc ();
			} while (get_reg (RTL8029_BOUNDARY) != current_point);

			put_reg (RTL8029_TRANSMITCONFIGURATION, 0xe0);
		}

		if (temp & 0x1) {
			udelay (10 * 1000);

			/*packet received */
			do {
				put_reg (RTL8029_INTERRUPTSTATUS, 0x01);
				current_point = nic_to_pc ();
			} while (get_reg (RTL8029_BOUNDARY) != current_point);
		}

		if (!(temp & 0x1))
			return 0;

		


		/* done and exit. */
	}
}

/* Send a data block via Ethernet. */
extern int rtl8029_eth_send (struct eth_device *dev,volatile void *packet, int length)
{
	volatile unsigned char *p;
	unsigned int pn;

	pn = length;
	p = (volatile unsigned char *) packet;

	while (get_reg (RTL8029_COMMAND) == RTL8029_TRANSMIT);

	put_reg (RTL8029_REMOTESTARTADDRESS0, 0);
	put_reg (RTL8029_REMOTESTARTADDRESS1, RTL8029_TPSTART);
	put_reg (RTL8029_REMOTEBYTECOUNT0, (pn & 0xff));
	put_reg (RTL8029_REMOTEBYTECOUNT1, ((pn >> 8) & 0xff));

	put_reg (RTL8029_COMMAND, RTL8029_REMOTEDMAWR);
	while (pn > 0) {
		put_reg (RTL8029_DMA_DATA, *p++);
		pn--;
	}

	pn = length;

	while (pn < 60) {	/*Padding */
		put_reg (RTL8029_DMA_DATA, 0);
		pn++;
	}

	while (!(get_reg (RTL8029_INTERRUPTSTATUS)) & 0x40);

	put_reg (RTL8029_INTERRUPTSTATUS, 0x40);
	put_reg (RTL8029_TRANSMITPAGE, RTL8029_TPSTART);
	put_reg (RTL8029_TRANSMITBYTECOUNT0, (pn & 0xff));
	put_reg (RTL8029_TRANSMITBYTECOUNT1, ((pn >> 8 & 0xff)));
	put_reg (RTL8029_COMMAND, RTL8029_TRANSMIT);

	udelay (10 * 1000);

	return 0;
}


static struct pci_device_id supported[] = {
       {PCI_VENDOR_ID_REALTEK, PCI_DEVICE_ID_REALTEK_8029},
       {}
};

int rtl8029_initialize(bd_t *bis)
{
	pci_dev_t devno;
	int card_number = 0;

	u32 iobase;
	int idx=0;

	while(1){
		/* Find RTL8029 */
		if ((devno = pci_find_devices(supported, idx++)) < 0)
			break;

		pci_read_config_dword(devno, PCI_BASE_ADDRESS_0, &iobase);
		iobase &= ~0xf;


		debug ("rtl8029: REALTEK RTL8029 @0x%x\n", iobase);

		rtl8029_dev = (struct eth_device *)malloc(sizeof *rtl8029_dev);

		sprintf (rtl8029_dev->name, "RTL8029#%d", card_number);

		rtl8029_dev->priv = (void *) devno;
		rtl8029_dev->iobase = (int)iobase;
		rtl8029_dev->init = rtl8029_eth_init;
		rtl8029_dev->halt = rtl8029_eth_halt;
		rtl8029_dev->send = rtl8029_eth_send;
		rtl8029_dev->recv = rtl8029_eth_rx;

		eth_register (rtl8029_dev);

		card_number++;

		pci_write_config_byte (devno, PCI_LATENCY_TIMER, 0x20);

		udelay (10 * 1000);
	}

	return card_number;
}


#endif /* COMMANDS & CFG_NET */

#endif /* CONFIG_DRIVER_RTL8029 */