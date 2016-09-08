/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 */
#include <linux/console.h>
#include <linux/init.h>
#include <linux/serial_reg.h>
#include <asm/io.h>


#define SEAD_UART1_REGS_BASE    0xbf000800   /* ttyS1 = RS232 port */
#define SEAD_UART0_REGS_BASE    0xbf000900   /* ttyS0 = USB port   */

#define PORT(base_addr, offset) ((unsigned int __iomem *)(base_addr+(offset)*4))

static inline unsigned int serial_in(int offset, unsigned int base_addr)
{
	return __raw_readl(PORT(base_addr, offset)) & 0xff;
}

static inline void serial_out(int offset, int value, unsigned int base_addr)
{
	__raw_writel(value, PORT(base_addr, offset));
}

int prom_putchar(char c, char port)
{
	unsigned int base_addr;

	base_addr = port ? SEAD_UART1_REGS_BASE : SEAD_UART0_REGS_BASE;

	while ((serial_in(UART_LSR, base_addr) & UART_LSR_THRE) == 0)
		;

	serial_out(UART_TX, c, base_addr);

	return 1;
}
