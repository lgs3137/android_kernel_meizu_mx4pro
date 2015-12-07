/* arch/arm/mach-s3c2410/include/mach/regs-watchdog.h
 *
 * Copyright (c) 2003 Simtec Electronics <linux@simtec.co.uk>
 *		      http://www.simtec.co.uk/products/SWLINUX/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2410 Watchdog timer control
*/


#ifndef __ASM_ARCH_REGS_WATCHDOG_H
#define __ASM_ARCH_REGS_WATCHDOG_H

#define S3C_WDOGREG(x) ((x) + S3C_VA_WATCHDOG)

#define S3C2410_WTCON	   S3C_WDOGREG(0x00)
#define S3C2410_WTDAT	   S3C_WDOGREG(0x04)
#define S3C2410_WTCNT	   S3C_WDOGREG(0x08)
#define S3C2410_WTCLRINT   S3C_WDOGREG(0x0C)

/* the watchdog can either generate a reset pulse, or an
 * interrupt.
 */

#define S3C2410_WTCON_RSTEN   (0x01)
#define S3C2410_WTCON_INTEN   (1<<2)
#define S3C2410_WTCON_ENABLE  (1<<5)

#define S3C2410_WTCON_DIV16   (0<<3)
#define S3C2410_WTCON_DIV32   (1<<3)
#define S3C2410_WTCON_DIV64   (2<<3)
#define S3C2410_WTCON_DIV128  (3<<3)
#define S3C2410_WTCON_DIVMAX  (128)

#define S3C2410_WTCON_PRESCALE(x) ((x) << 8)
#define S3C2410_WTCON_PRESCALE_MASK (0xff00)
#define S3C2410_WTCON_PRESCALE_MAX  (0xFF)

#define S3C2410_WTCNT_MAX     (0xFFFF)

void s3c2410wdt_save(void);
void s3c2410wdt_restore(void);

#define watchdog_save()         s3c2410wdt_save();
#define watchdog_restore()      s3c2410wdt_restore();

#endif /* __ASM_ARCH_REGS_WATCHDOG_H */


